"""
CalcForge Syntax Highlighter - Backend Logic for Syntax Highlighting
Converts Qt-based highlighting to CSS classes for web frontend.
"""

import re
from typing import List, Dict, Any, Optional

from constants import FUNCTION_NAMES, LN_COLORS, COLORS


class SyntaxHighlighter:
    """
    Syntax highlighter that generates CSS class information for frontend.
    Replaces Qt-based highlighting with web-compatible data structures.
    """
    
    def __init__(self):
        # Color palette for LN variables
        self.ln_colors = LN_COLORS
        
        # Store persistent LN colors
        self.persistent_ln_colors = {}
        
        # Function names for highlighting
        self.function_names = FUNCTION_NAMES
        
        # CSS class mappings
        self.css_classes = {
            'number': 'syntax-number',
            'operator': 'syntax-operator', 
            'function': 'syntax-function',
            'paren': 'syntax-paren',
            'unmatched': 'syntax-unmatched',
            'comment': 'syntax-comment',
            'ln_reference': 'syntax-ln-ref',
            'sheet_reference': 'syntax-sheet-ref',
            'error': 'syntax-error'
        }
    
    def get_ln_color(self, ln_number: int) -> str:
        """Get or assign a color for an LN variable"""
        if ln_number not in self.persistent_ln_colors:
            # Assign a new color from the palette
            color_idx = len(self.persistent_ln_colors) % len(self.ln_colors)
            self.persistent_ln_colors[ln_number] = self.ln_colors[color_idx]
        return self.persistent_ln_colors[ln_number]
    
    def highlight_text(self, text: str) -> List[Dict[str, Any]]:
        """
        Generate syntax highlighting data for text.
        
        Args:
            text (str): Text to highlight
            
        Returns:
            list: List of highlight ranges with CSS classes and colors
        """
        highlights = []
        
        if not text.strip():
            return highlights
        
        # Process line by line for proper highlighting
        lines = text.split('\n')
        current_pos = 0

        for line in lines:
            line_start = current_pos
            line_length = len(line)

            # Handle comment lines
            if line.strip().startswith(":::"):
                # FORCE the correct green color - bypass any import issues
                correct_green_color = "#7ED321"
                print(f"DEBUG: Creating comment highlight with FORCED color: {correct_green_color}")
                highlights.append({
                    "start": line_start,
                    "length": line_length,
                    "class": self.css_classes['comment'],
                    "color": correct_green_color
                })
            else:
                # Process non-comment lines for syntax highlighting
                line_highlights = self.highlight_line(line, line_start)
                highlights.extend(line_highlights)

            # Move to next line (including newline character)
            current_pos += line_length + 1

        return highlights

    def highlight_line(self, line, line_start):
        """
        Highlight a single line of text
        """
        highlights = []

        if not line.strip():
            return highlights

        # Highlight numbers
        for match in re.finditer(r"\b\d+(?:\.\d+)?\b", line):
            highlights.append({
                "start": line_start + match.start(),
                "length": match.end() - match.start(),
                "class": self.css_classes['number'],
                "color": COLORS['number']
            })

        # Highlight operators (including 'to' keyword)
        for match in re.finditer(r"\bto\b|[+\-*/%^=]", line):
            highlights.append({
                "start": line_start + match.start(),
                "length": match.end() - match.start(),
                "class": self.css_classes['operator'],
                "color": COLORS['operator']
            })

        # Highlight function names
        for func_name in self.function_names:
            # Create regex pattern for function name followed by opening parenthesis
            pattern = r"\b" + re.escape(func_name) + r"\b(?=\s*\()"
            for match in re.finditer(pattern, line, re.IGNORECASE):
                highlights.append({
                    "start": line_start + match.start(),
                    "length": match.end() - match.start(),
                    "class": self.css_classes['function'],
                    "color": COLORS['function']
                })

        # Highlight parentheses and check for matching
        paren_highlights = self._highlight_parentheses_in_line(line, line_start)
        highlights.extend(paren_highlights)

        # Highlight cross-sheet references first (they take precedence)
        sheet_highlights = self._highlight_sheet_references_in_line(line, line_start)
        highlights.extend(sheet_highlights)

        # Highlight regular LN references
        ln_highlights = self._highlight_ln_references_in_line(line, line_start)
        highlights.extend(ln_highlights)

        # Sort highlights by start position to ensure proper ordering
        highlights.sort(key=lambda x: x['start'])

        return highlights

    def _highlight_parentheses_in_line(self, line: str, line_start: int) -> List[Dict[str, Any]]:
        """Highlight parentheses in a single line"""
        highlights = []
        stack = []
        pairs = []

        # Find matching parentheses
        for i, ch in enumerate(line):
            if ch == '(':
                stack.append(i)
            elif ch == ')' and stack:
                start = stack.pop()
                pairs.append((start, i))

        # Highlight matched pairs
        for start, end in pairs:
            highlights.append({
                "start": line_start + start,
                "length": 1,
                "class": self.css_classes['paren'],
                "color": COLORS['paren']
            })
            highlights.append({
                "start": line_start + end,
                "length": 1,
                "class": self.css_classes['paren'],
                "color": COLORS['paren']
            })

        # Highlight unmatched opening parentheses
        for pos in stack:
            highlights.append({
                "start": line_start + pos,
                "length": 1,
                "class": self.css_classes['unmatched'],
                "color": COLORS['unmatched']
            })

        return highlights

    def _highlight_sheet_references_in_line(self, line: str, line_start: int) -> List[Dict[str, Any]]:
        """Highlight cross-sheet references in a single line"""
        highlights = []

        # Pattern for cross-sheet references (case insensitive)
        pattern = r"\bS\.(.*?)\.LN(\d+)\b"
        for match in re.finditer(pattern, line, re.IGNORECASE):
            sheet_name = match.group(1)
            ln_num = int(match.group(2))

            # Get color for the LN number
            ln_color = self.get_ln_color(ln_num)

            # Highlight the entire reference
            highlights.append({
                "start": line_start + match.start(),
                "length": match.end() - match.start(),
                "class": self.css_classes['sheet_reference'],
                "color": ln_color,
                "ln_number": ln_num,
                "sheet_name": sheet_name
            })

            # Add special highlighting for the sheet name part
            sheet_start = line_start + match.start() + 2  # Skip "S."
            sheet_length = len(sheet_name)
            highlights.append({
                "start": sheet_start,
                "length": sheet_length,
                "class": "syntax-sheet-name",
                "color": COLORS['sheet_ref']
            })

        return highlights

    def _highlight_ln_references_in_line(self, line: str, line_start: int) -> List[Dict[str, Any]]:
        """Highlight regular LN references in a single line"""
        highlights = []

        # Pattern for regular LN references (case insensitive)
        pattern = r"\bLN(\d+)\b"
        for match in re.finditer(pattern, line, re.IGNORECASE):
            ln_num = int(match.group(1))
            ln_color = self.get_ln_color(ln_num)

            highlights.append({
                "start": line_start + match.start(),
                "length": match.end() - match.start(),
                "class": self.css_classes['ln_reference'],
                "color": ln_color,
                "ln_number": ln_num
            })

        return highlights

    def _highlight_parentheses(self, text: str) -> List[Dict[str, Any]]:
        """Highlight parentheses and mark unmatched ones"""
        highlights = []
        stack = []
        pairs = []
        
        # Find matching parentheses
        for i, ch in enumerate(text):
            if ch == '(':
                stack.append(i)
            elif ch == ')' and stack:
                start = stack.pop()
                pairs.append((start, i))
        
        # Highlight matched pairs
        for start, end in pairs:
            highlights.append({
                "start": start,
                "length": 1,
                "class": self.css_classes['paren'],
                "color": COLORS['paren']
            })
            highlights.append({
                "start": end,
                "length": 1,
                "class": self.css_classes['paren'],
                "color": COLORS['paren']
            })
        
        # Highlight unmatched opening parentheses
        for pos in stack:
            highlights.append({
                "start": pos,
                "length": 1,
                "class": self.css_classes['unmatched'],
                "color": COLORS['unmatched']
            })
        
        return highlights
    
    def _highlight_sheet_references(self, text: str) -> List[Dict[str, Any]]:
        """Highlight cross-sheet references like S.SheetName.LN5"""
        highlights = []
        
        # Pattern for cross-sheet references (case insensitive)
        pattern = r"\bS\.(.*?)\.LN(\d+)\b"
        for match in re.finditer(pattern, text, re.IGNORECASE):
            sheet_name = match.group(1)
            ln_num = int(match.group(2))
            
            # Get color for the LN number
            ln_color = self.get_ln_color(ln_num)
            
            # Highlight the entire reference
            highlights.append({
                "start": match.start(),
                "length": match.end() - match.start(),
                "class": self.css_classes['sheet_reference'],
                "color": ln_color,
                "ln_number": ln_num,
                "sheet_name": sheet_name
            })
            
            # Add special highlighting for the sheet name part
            sheet_start = match.start() + 2  # Skip "S."
            sheet_length = len(sheet_name)
            highlights.append({
                "start": sheet_start,
                "length": sheet_length,
                "class": "syntax-sheet-name",
                "color": COLORS['sheet_ref']
            })
        
        return highlights
    
    def _highlight_ln_references(self, text: str) -> List[Dict[str, Any]]:
        """Highlight regular LN references like LN5"""
        highlights = []
        
        # Pattern for regular LN references (case insensitive)
        pattern = r"\bLN(\d+)\b"
        for match in re.finditer(pattern, text, re.IGNORECASE):
            ln_num = int(match.group(1))
            ln_color = self.get_ln_color(ln_num)
            
            highlights.append({
                "start": match.start(),
                "length": match.end() - match.start(),
                "class": self.css_classes['ln_reference'],
                "color": ln_color,
                "ln_number": ln_num
            })
        
        return highlights
    
    def get_css_styles(self) -> str:
        """
        Generate CSS styles for syntax highlighting.
        
        Returns:
            str: CSS stylesheet for syntax highlighting
        """
        css = """
/* CalcForge Syntax Highlighting Styles */
.syntax-number {
    color: """ + COLORS['number'] + """;
}

.syntax-operator {
    color: """ + COLORS['operator'] + """;
}

.syntax-function {
    color: """ + COLORS['function'] + """;
}

.syntax-paren {
    color: """ + COLORS['paren'] + """;
}

.syntax-unmatched {
    color: """ + COLORS['unmatched'] + """;
    background-color: rgba(248, 81, 73, 0.2);
}

.syntax-comment {
    color: """ + COLORS['comment'] + """;
    font-style: italic;
}

.syntax-ln-ref {
    font-weight: bold;
}

.syntax-sheet-ref {
    font-weight: bold;
}

.syntax-sheet-name {
    color: """ + COLORS['sheet_ref'] + """;
}

.syntax-error {
    color: """ + COLORS['error'] + """;
    background-color: rgba(248, 81, 73, 0.2);
}
"""
        return css
    
    def reset_ln_colors(self):
        """Reset LN color assignments"""
        self.persistent_ln_colors.clear()
    
    def get_ln_color_map(self) -> Dict[int, str]:
        """Get current LN number to color mapping"""
        return self.persistent_ln_colors.copy()
