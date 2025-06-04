import sys, os, json, re, math
from pathlib import Path
from collections import Counter
from datetime import datetime, timedelta
import calendar
import statistics  # Add statistics import at top level
import time
import traceback

import pint
ureg = pint.UnitRegistry()

# Import constants from external module
from constants import (
    FALLBACK_RATES, CURRENCY_ABBR, CURRENCY_DISPLAY,
    UNIT_ABBR, UNIT_DISPLAY, MATH_FUNCS, COLORS, LN_COLORS, 
    FUNCTION_NAMES, DEFAULT_FPS, lcm
)

# Add currency conversion support
try:
    import requests
    CURRENCY_API_AVAILABLE = True
except ImportError:
    CURRENCY_API_AVAILABLE = False

class TimecodeError(Exception):
    pass


def parse_date(date_str):
    """Parse a date string in various formats"""
    date_str = date_str.strip()
    
    # First try to parse continuous number format (MMDDYYYY or MDYYYY)
    num_only = re.sub(r'[^\d]', '', date_str)
    if len(num_only) in (6, 7, 8):
        try:
            # Handle different length formats
            if len(num_only) == 8:  # MMDDYYYY
                month = int(num_only[:2])
                day = int(num_only[2:4])
                year = int(num_only[4:])
            elif len(num_only) == 7:  # MDDYYYY
                month = int(num_only[0])
                day = int(num_only[1:3])
                year = int(num_only[3:])
            else:  # MDYYYY
                month = int(num_only[0])
                day = int(num_only[1])
                year = int(num_only[2:])
            
            # Validate month and day
            if 1 <= month <= 12 and 1 <= day <= 31:
                return datetime(year, month, day).date()
        except ValueError:
            pass
    
    # If continuous format fails, try standard formats
    formats = [
        # Month name formats (try these first)
        '%B %d, %Y',  # "July 12, 1985"
        '%B %d,%Y',   # "July 12,1985"
        '%b %d, %Y',  # "Jul 12, 1985"
        '%b %d,%Y',   # "Jul 12,1985"
        # Numeric formats with different separators
        '%m/%d/%Y',   # "07/12/1985"
        '%m.%d.%Y',   # "07.12.1985"
        '%-m/%-d/%Y', # "7/12/1985"
        '%-m.%-d.%Y'  # "7.12.1985"
    ]
    
    for fmt in formats:
        try:
            return datetime.strptime(date_str, fmt).date()
        except ValueError:
            continue
            
    raise ValueError(f"Could not parse date: {date_str}")

def add_business_days(start_date, days):
    """Add business days to a date, skipping weekends"""
    current_date = start_date
    remaining_days = abs(days)
    direction = 1 if days > 0 else -1
    
    while remaining_days > 0:
        current_date += timedelta(days=direction)
        # Skip weekends (5 = Saturday, 6 = Sunday)
        if current_date.weekday() < 5:
            remaining_days -= 1
            
    return current_date

def count_business_days(start_date, end_date):
    """Count business days between two dates, excluding weekends"""
    if start_date > end_date:
        start_date, end_date = end_date, start_date
        
    business_days = 0
    current_date = start_date
    
    while current_date <= end_date:
        if current_date.weekday() < 5:  # Monday = 0, Sunday = 6
            business_days += 1
        current_date += timedelta(days=1)
        
    return business_days

def handle_date_arithmetic(expr):
    """Handle date arithmetic expressions inside D() functions"""
    # Define date patterns without D. prefixes
    date_patterns = [
        # Two dates with subtraction - handle spaces in dates and W- syntax
        r'([A-Za-z]+\s+\d+,\s*\d{4}|\d[\d.]*)\s*W\s*-\s*([A-Za-z]+\s+\d+,\s*\d{4}|\d[\d.]*)',  # Date range with W-
        r'([A-Za-z]+\s+\d+,\s*\d{4}|\d[\d.]*)\s*-\s*([A-Za-z]+\s+\d+,\s*\d{4}|\d[\d.]*)',       # Date range without W
        # Date plus/minus days - handle spaces and optional W
        r'([A-Za-z]+\s+\d+,\s*\d{4}|\d[\d.]*)\s*W\s*([+\-])\s*(\d+)',  # With W
        r'([A-Za-z]+\s+\d+,\s*\d{4}|\d[\d.]*)\s*([+\-])\s*(\d+)',      # Without W
        # Single date
        r'^([A-Za-z]+\s+\d+,\s*\d{4}|\d[\d.]*)$'
    ]
    
    for pattern in date_patterns:
        match = re.match(pattern, expr.strip())
        if match:
            groups = match.groups()
            
            if len(groups) == 2:  # Date range
                try:
                    date1 = parse_date(groups[0].strip())
                    date2 = parse_date(groups[1].strip())
                    # Check if there's a W before the minus sign
                    if 'W-' in expr or 'W -' in expr:
                        days = count_business_days(date1, date2)
                        return {'value': days, 'unit': 'Business Days'}
                    else:
                        days = (date2 - date1).days
                        return {'value': days, 'unit': 'Days'}
                except ValueError as e:
                    return None
                    
            elif len(groups) == 3:  # Date arithmetic
                try:
                    date = parse_date(groups[0].strip())
                    op = groups[1]
                    days = int(groups[2])
                    
                    # Check if this is a business day calculation
                    if 'W' in expr:
                        if op == '+':
                            result = add_business_days(date, days)
                        else:
                            result = add_business_days(date, -days)
                    else:
                        if op == '+':
                            result = date + timedelta(days=days)
                        else:
                            result = date - timedelta(days=days)
                            
                    return result.strftime('%B %d, %Y')
                except ValueError as e:
                    return None
                    
            elif len(groups) == 1:  # Single date
                try:
                    date = parse_date(groups[0].strip())
                    return date.strftime('%B %d, %Y')
                except ValueError as e:
                    return None
                    
    return None

def parse_timecode(tc_str):
    """Parse a timecode string into hours, minutes, seconds, frames"""
    # Replace periods with colons for consistent parsing
    tc_str = tc_str.replace('.', ':')
    parts = tc_str.split(':')
    
    if len(parts) != 4:
        raise TimecodeError(f"Invalid timecode format: {tc_str}. Expected HH:MM:SS:FF")
    
    try:
        hours = int(parts[0])
        minutes = int(parts[1])
        seconds = int(parts[2])
        frames = int(parts[3])
        
        # Validate ranges
        if not (0 <= hours and 0 <= minutes < 60 and 0 <= seconds < 60 and 0 <= frames):
            raise TimecodeError(f"Invalid timecode values in: {tc_str}")
        
        return hours, minutes, seconds, frames
    except ValueError:
        raise TimecodeError(f"Invalid number in timecode: {tc_str}")

def timecode_to_frames(tc_str, fps):
    """Convert a timecode string to total frames"""
    if isinstance(tc_str, (int, float)):
        return int(tc_str)
        
    hours, minutes, seconds, frames = parse_timecode(tc_str)
    
    # Validate frame count against fps
    max_frames = int(fps) if fps == int(fps) else int(fps) + 1
    if frames >= max_frames:
        raise TimecodeError(f"Frame count {frames} exceeds maximum for {fps} fps (max: {max_frames-1})")
    
    if abs(fps - 29.97) < 0.01:
        # For 29.97 fps drop frame:
        # Calculate total frames using the standard drop frame formula
        total_minutes = (60 * hours) + minutes
        
        # Calculate base frames (as if 30fps)
        total_frames = (hours * 3600 * 30) + (minutes * 60 * 30) + (seconds * 30) + frames
        
        # Subtract dropped frames: 2 frames dropped every minute except every 10th minute
        drops = 2 * (total_minutes - total_minutes // 10)
        
        result = total_frames - drops
        return result
        
    elif abs(fps - 59.94) < 0.01:
        # For 59.94 fps drop frame
        total_minutes = (60 * hours) + minutes
        
        # Calculate base frames (as if 60fps)
        total_frames = (hours * 3600 * 60) + (minutes * 60 * 60) + (seconds * 60) + frames
        
        # Subtract dropped frames: 4 frames dropped every minute except every 10th minute
        drops = 4 * (total_minutes - total_minutes // 10)
        
        result = total_frames - drops
        return result
        
    elif abs(fps - 23.976) < 0.01:
        # For 23.976, use exact NTSC frame rate
        exact_fps = 24000 / 1001
        total_seconds = hours * 3600 + minutes * 60 + seconds
        result = int(round(total_seconds * exact_fps)) + frames
        return result
    else:
        # Non-drop frame rates - simple calculation
        total_seconds = hours * 3600 + minutes * 60 + seconds
        result = int(total_seconds * fps) + frames
        return result

def frames_to_timecode(frame_count, fps):
    """Convert frame count to timecode string"""
    if frame_count < 0:
        sign = "-"
        frame_count = abs(frame_count)
    else:
        sign = ""
    
    if abs(fps - 29.97) < 0.01:
        # For 29.97 drop frame, first calculate total minutes
        total_minutes = frame_count // (30 * 60)  # Using 30 fps as base
        
        # Calculate the number of drop frames
        drops = 2 * (total_minutes - total_minutes // 10)
        
        # Add back the dropped frames to get real frame count
        real_frames = frame_count + drops
        
        # Now calculate the time components
        frames = real_frames % 30
        total_seconds = real_frames // 30
        seconds = total_seconds % 60
        total_minutes = total_seconds // 60
        hours = total_minutes // 60
        minutes = total_minutes % 60
        
    elif abs(fps - 59.94) < 0.01:
        # Similar logic for 59.94
        total_minutes = frame_count // (60 * 60)
        drops = 4 * (total_minutes - total_minutes // 10)
        real_frames = frame_count + drops
        
        frames = real_frames % 60
        total_seconds = real_frames // 60
        seconds = total_seconds % 60
        total_minutes = total_seconds // 60
        hours = total_minutes // 60
        minutes = total_minutes % 60
        
    else:
        # Non-drop frame rates
        if abs(fps - 23.976) < 0.01:
            exact_fps = 24000 / 1001
            total_seconds = frame_count / exact_fps
        else:
            total_seconds = frame_count / fps
            
        hours = int(total_seconds // 3600)
        minutes = int((total_seconds % 3600) // 60)
        seconds = int(total_seconds % 60)
        frames = int(round((total_seconds % 1) * fps))
        
        # Handle frame overflow
        if frames >= int(round(fps)):
            frames = 0
            seconds += 1
            if seconds >= 60:
                seconds = 0
                minutes += 1
                if minutes >= 60:
                    minutes = 0
                    hours += 1
    
    return f"{sign}{hours:02d}:{minutes:02d}:{seconds:02d}:{frames:02d}"

def evaluate_timecode_expr(fps, expr):
    """Evaluate a timecode expression"""
    # If it's just a number, convert it to timecode
    if isinstance(expr, (int, float)):
        return frames_to_timecode(int(expr), fps)
    
    # Normalize all timecode separators to colons
    expr = expr.replace('.', ':')
    
    # Find all timecodes and operators in the expression
    tc_pattern = r'\d{1,2}[:\.]\d{2}[:\.]\d{2}[:\.]\d{2}'
    
    # First, standardize the expression by adding spaces around operators
    expr = re.sub(r'([+\-*/])', r' \1 ', expr.strip())
    
    # Split the expression into tokens
    tokens = expr.split()
    
    # Process each token
    result = None
    current_op = '+'
    
    for token in tokens:
        if token in '+-*/':
            current_op = token
            continue
            
        try:
            # Try to convert token to frames
            if re.match(r'^\d{1,2}[:\.]\d{1,2}[:\.]\d{1,2}[:\.]\d{1,2}$', token):
                frames = timecode_to_frames(token, fps)
            else:
                # If not a timecode, evaluate as a number
                frames = float(token)
                
            # Apply operation
            if result is None:
                result = frames
            else:
                if current_op == '+':
                    result += frames
                elif current_op == '-':
                    result -= frames
                elif current_op == '*':
                    result *= frames
                elif current_op == '/':
                    result /= frames
        except Exception as e:
            raise TimecodeError(f"Error in timecode expression: {str(e)}")
    
    if result is None:
        raise TimecodeError("No valid timecode or numeric values found in expression")
        
    return frames_to_timecode(int(round(result)), fps)

def TC(fps, *args):
    """Main timecode function that handles both conversion and arithmetic"""
    if not args:
        raise TimecodeError("TC function requires at least two arguments: framerate and timecode/frames")
    
    # Join all arguments to handle expressions with spaces
    expr = ' '.join(str(arg) for arg in args)
    
    try:
        fps = float(fps)
        if fps <= 0:
            raise TimecodeError("Framerate must be positive")
        
        # Clean up the expression
        expr = expr.strip()
        
        # If it's a simple number, convert frames to timecode
        if expr.isdigit():
            return frames_to_timecode(int(expr), fps)
            
        # If it's a single timecode without arithmetic, normalize separators and return frames
        # Handle both : and . as separators, and allow optional leading 0 in hours
        if re.match(r'^\d{1,2}[:\.]\d{1,2}[:\.]\d{1,2}[:\.]\d{1,2}$', expr):
            # Normalize to use colons
            expr = expr.replace('.', ':')
            frames = timecode_to_frames(expr, fps)
            return str(frames)
            
        # Handle timecode arithmetic
        return evaluate_timecode_expr(fps, expr)
    except Exception as e:
        raise TimecodeError(f"Error in TC function: {str(e)}")

def AR(original, target):
    """Aspect ratio calculator function"""
    try:
        # Convert inputs to strings and clean them up
        original_str = str(original).strip()
        target_str = str(target).strip()
        
        # Parse original dimensions (e.g., "1920x1080")
        original_match = re.match(r'(\d+(?:\.\d+)?)x(\d+(?:\.\d+)?)', original_str, re.IGNORECASE)
        if not original_match:
            raise ValueError(f"Invalid original dimensions format: {original_str}")
        
        orig_width = float(original_match.group(1))
        orig_height = float(original_match.group(2))
        aspect_ratio = orig_width / orig_height
        
        # Parse target dimensions (e.g., "?x2000" or "1280x?")
        target_match = re.match(r'(\?|\d+(?:\.\d+)?)x(\?|\d+(?:\.\d+)?)', target_str, re.IGNORECASE)
        if not target_match:
            raise ValueError(f"Invalid target dimensions format: {target_str}")
        
        target_width_str = target_match.group(1)
        target_height_str = target_match.group(2)
        
        # Solve for the missing dimension
        if target_width_str == '?' and target_height_str != '?':
            # Solve for width: width = height * aspect_ratio
            target_height = float(target_height_str)
            result_width = target_height * aspect_ratio
            result = f"{result_width:.0f}x{target_height:.0f}"
            return result
            
        elif target_height_str == '?' and target_width_str != '?':
            # Solve for height: height = width / aspect_ratio
            target_width = float(target_width_str)
            result_height = target_width / aspect_ratio
            result = f"{target_width:.0f}x{result_height:.0f}"
            return result
            
        else:
            raise ValueError("Exactly one dimension must be '?' to solve for")
            
    except Exception as e:
        raise ValueError(f"Aspect ratio calculation error: {str(e)}")


def truncate(value, decimals=2):
    """Rounds a number to specified decimal places"""
    if isinstance(value, str):
        # If it's a string expression, evaluate it first
        try:
            value = eval(value, {"truncate": truncate, "TR": truncate, **GLOBALS}, {})
        except:
            return value
    if isinstance(value, dict) and 'value' in value:
        # Handle unit conversion results
        truncated_value = round(value['value'] * (10 ** decimals)) / (10 ** decimals)
        # If decimals=0, convert to integer to avoid .0 suffix
        if decimals == 0:
            truncated_value = int(truncated_value)
        return {'value': truncated_value, 'unit': value['unit']}
    if not isinstance(value, (int, float)):
        return value
    factor = 10 ** decimals
    result = round(value * factor) / factor
    # If decimals=0, return as integer to avoid .0 suffix that confuses TC function
    if decimals == 0:
        result = int(result)
    return result


# Add TC function and math functions to evaluation namespace
GLOBALS = {"TC": TC, "AR": AR, "truncate": truncate, "TR": truncate, **MATH_FUNCS}

from PySide6.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QHBoxLayout, QPlainTextEdit,
    QTextEdit, QSplitter, QPushButton, QMessageBox, QTabWidget, QInputDialog,
    QToolTip, QCompleter, QListWidget, QCheckBox, QDialog, QLabel
)
from PySide6.QtGui import (
    QFont, QSyntaxHighlighter, QTextCharFormat, QColor,
    QTextCursor, QPainter, QPalette, QTextBlockUserData, QStandardItemModel,
    QStandardItem, QIcon, QPen
)
from PySide6.QtCore import (
    Qt, QTimer, QRegularExpression, QSize, QRect, Slot, QSettings, QEvent,
    QStringListModel, QObject, QPoint
)

# Update the unit abbreviation mapping
# All constants moved to constants.py module

def get_exchange_rate(from_currency, to_currency):
    """Get exchange rate between two currencies"""
    if from_currency == to_currency:
        return 1.0
    
    # Try to get real-time rates first
    if CURRENCY_API_AVAILABLE:
        try:
            # Using a free API - exchangerate.host
            url = f"https://api.exchangerate.host/latest?base={from_currency.upper()}&symbols={to_currency.upper()}"
            response = requests.get(url, timeout=3)
            if response.status_code == 200:
                data = response.json()
                if 'rates' in data and to_currency.upper() in data['rates']:
                    return data['rates'][to_currency.upper()]
        except Exception as e:
            pass
    
    # Fall back to static rates
    from_rate = FALLBACK_RATES.get(from_currency.lower(), None)
    to_rate = FALLBACK_RATES.get(to_currency.lower(), None)
    
    if from_rate is None or to_rate is None:
        return None
    
    # Convert via USD
    return to_rate / from_rate

def handle_currency_conversion(expr):
    """Handle currency conversion expressions like '20.40 dollars to euros'"""
    # Pattern to match currency conversions
    pattern = r'^([\d.]+)\s+(.+?)\s+to\s+(.+?)$'
    match = re.match(pattern, expr.strip(), re.IGNORECASE)
    
    if match:
        value, from_currency, to_currency = match.groups()
        
        # Clean up currency names and get abbreviations
        from_currency = from_currency.lower().strip()
        to_currency = to_currency.lower().strip()
        
        # Convert to standard abbreviations
        from_abbr = CURRENCY_ABBR.get(from_currency)
        to_abbr = CURRENCY_ABBR.get(to_currency)
        
        if from_abbr and to_abbr:
            try:
                value = float(value)
                rate = get_exchange_rate(from_abbr, to_abbr)
                
                if rate is not None:
                    result = value * rate
                    # Get display name for the target currency
                    display_currency = CURRENCY_DISPLAY.get(to_abbr, to_currency)
                    return {'value': result, 'unit': display_currency}
            except (ValueError, TypeError):
                pass
    
    return None

def remove_thousands_commas(match):
    """Remove thousands separators (commas) from number strings"""
    number_str = match.group(0)
    # Remove all commas from the number
    return number_str.replace(',', '')

def repl_num(m):
    """Replace numbers with leading zeros, avoiding timecodes and quoted strings"""
    # Don't replace if it's part of a timecode
    if re.match(r'\d{1,2}[:.]\d{1,2}[:.]\d{1,2}[:.]\d{1,2}', m.string[max(0, m.start()-8):m.end()+8]):
        return m.group(0)
    # Don't replace if it's inside quotes
    before_match = m.string[:m.start()]
    quote_count_before = before_match.count('"') - before_match.count('\\"')
    if quote_count_before % 2 == 1:  # We're inside quotes
        return m.group(0)
    return str(int(m.group(1)))

class LineData(QTextBlockUserData):
    def __init__(self, id):
        super().__init__()
        self.id = id

class LineNumberAreaBase(QWidget):
    """Base class for line number areas with common functionality"""
    
    def __init__(self, parent):
        super().__init__(parent)
        self.widget = parent  # Generic reference to the parent widget
    
    def sizeHint(self):
        return QSize(self.lineNumberAreaWidth(), 0)
    
    def lineNumberAreaWidth(self):
        """Calculate width needed for line numbers - override in subclasses"""
        raise NotImplementedError("Subclasses must implement lineNumberAreaWidth")
    
    def get_current_line_number(self):
        """Get the current line number for highlighting - override in subclasses"""
        return -1
    
    def get_line_label_and_color(self, block, block_number):
        """Get the label and color for a line - override in subclasses"""
        return str(block_number + 1), "#888"
    
    def paint_common_logic(self, event, widget):
        """Common painting logic shared between line number areas"""
        painter = QPainter(self)
        painter.fillRect(event.rect(), QColor("#1e1e1e"))
        block = widget.firstVisibleBlock()
        top = widget.blockBoundingGeometry(block).translated(widget.contentOffset()).top()
        bottom = top + widget.blockBoundingRect(block).height()
        
        current_block_number = self.get_current_line_number()
        
        while block.isValid() and top <= event.rect().bottom():
            if block.isVisible() and bottom >= event.rect().top():
                block_number = block.blockNumber()
                label, color = self.get_line_label_and_color(block, block_number)
                
                # Check if this is the current line - make it bold and white
                is_current_line = block_number == current_block_number
                if is_current_line:
                    color = "#FFFFFF"  # White for current line
                    font = painter.font()
                    font.setBold(True)
                    painter.setFont(font)
                else:
                    font = painter.font()
                    font.setBold(False)
                    painter.setFont(font)
                
                painter.setPen(QColor(color))
                painter.drawText(0, int(top), self.width()-2, widget.fontMetrics().height(), Qt.AlignRight, label)
            
            block = block.next()
            top = bottom
            bottom = top + widget.blockBoundingRect(block).height()

class LineNumberArea(LineNumberAreaBase):
    def __init__(self, editor):
        super().__init__(editor)
        self.editor = editor
    
    def lineNumberAreaWidth(self):
        return self.editor.lineNumberAreaWidth()
    
    def paintEvent(self, event):
        self.editor.lineNumberAreaPaintEvent(event)

class ResultsLineNumberArea(LineNumberAreaBase):
    def __init__(self, results_widget):
        super().__init__(results_widget)
        self.results_widget = results_widget
    
    def lineNumberAreaWidth(self):
        digits = len(str(max(1, self.results_widget.blockCount())))
        return 3 + self.results_widget.fontMetrics().horizontalAdvance('9') * digits
    
    def get_current_line_number(self):
        # Get the current line number from the editor, not the results widget
        worksheet = self.results_widget.parent()
        while worksheet and not hasattr(worksheet, 'editor'):
            worksheet = worksheet.parent()
        
        if worksheet and hasattr(worksheet, 'editor'):
            return worksheet.editor.textCursor().blockNumber()
        return -1
    
    def get_line_label_and_color(self, block, block_number):
        label = str(block_number + 1)
        color = "#888"
        
        # Check if corresponding editor line exists and is a comment
        worksheet = self.results_widget.parent()
        while worksheet and not hasattr(worksheet, 'editor'):
            worksheet = worksheet.parent()
        
        if worksheet and hasattr(worksheet, 'editor'):
            editor_doc = worksheet.editor.document()
            if block_number < editor_doc.blockCount():
                editor_block = editor_doc.findBlockByNumber(block_number)
                if editor_block.isValid():
                    editor_text = editor_block.text().strip()
                    editor_data = editor_block.userData()
                    
                    if editor_text.startswith(":::"):
                        label = "C"
                        color = "#7ED321"
                    elif isinstance(editor_data, LineData):
                        label = str(editor_data.id)
        
        return label, color
    
    def paintEvent(self, event):
        self.paint_common_logic(event, self.results_widget)

class BaseHighlighter(QSyntaxHighlighter):
    """Base class for all syntax highlighters with common formatting functionality"""
    
    def __init__(self, document):
        super().__init__(document)
        
    def _fmt(self, color, bold=False, alpha=255):
        """Create a text format with given color and optional bold styling"""
        fmt = QTextCharFormat()
        fmt.setForeground(QColor(color))
        if bold:
            fmt.setFontWeight(QFont.Bold)
        return fmt
        
    def get_darker_color(self, color, factor=0.3):
        """Return a darker version of the given color for background highlighting"""
        c = QColor(color)
        h, s, v, a = c.getHsv()
        return QColor.fromHsv(h, s, int(v * factor), a)

class FormulaHighlighter(BaseHighlighter):
    """Advanced syntax highlighter for formula input with comprehensive highlighting"""
    
    def __init__(self, document):
        super().__init__(document)
        # Base colors for syntax - use constants from constants module
        self.formats = {
            'number': self._fmt(COLORS['number']),
            'operator': self._fmt(COLORS['operator']),
            'function': self._fmt(COLORS['function']),  # Functions use same blue as operators/to
            'paren': self._fmt(COLORS['paren']),
            'unmatched': self._fmt(COLORS['unmatched']),
            'comment': self._fmt(COLORS['comment']),
        }
        
        # Color palette for LN variables - use constants from constants module
        self.ln_colors = LN_COLORS
        
        # Store persistent LN colors
        self.persistent_ln_colors = {}
        
        # Use function names from constants module
        self.function_names = FUNCTION_NAMES
        
    def get_ln_color(self, ln_number):
        """Get or assign a color for an LN variable"""
        if ln_number not in self.persistent_ln_colors:
            # Assign a new color from the palette
            color_idx = len(self.persistent_ln_colors) % len(self.ln_colors)
            self.persistent_ln_colors[ln_number] = self.ln_colors[color_idx]
        return self.persistent_ln_colors[ln_number]
        
    def highlightBlock(self, text):
        if text.strip().startswith(":::"):
            self.setFormat(0, len(text), self.formats['comment'])
            return
            
        # First reset all formatting
        self.setFormat(0, len(text), QTextCharFormat())
        
        # Highlight numbers
        num_re = QRegularExpression(r"\b\d+(?:\.\d+)?\b")
        it = num_re.globalMatch(text)
        while it.hasNext():
            m = it.next()
            self.setFormat(m.capturedStart(), m.capturedLength(), self.formats['number'])
            
        # Highlight operators
        op_re = QRegularExpression(r"\bto\b|[+\-*/%^=]")
        it = op_re.globalMatch(text)
        while it.hasNext():
            m = it.next()
            self.setFormat(m.capturedStart(), m.capturedLength(), self.formats['operator'])
            
        # Highlight function names
        for func_name in self.function_names:
            # Create regex pattern for function name followed by opening parenthesis
            func_re = QRegularExpression(r"\b" + re.escape(func_name) + r"\b(?=\s*\()", QRegularExpression.CaseInsensitiveOption)
            it = func_re.globalMatch(text)
            while it.hasNext():
                m = it.next()
                self.setFormat(m.capturedStart(), m.capturedLength(), self.formats['function'])
            
        # Highlight parentheses
        stack = []
        pairs = []
        for i, ch in enumerate(text):
            if ch == '(':
                stack.append(i)
            elif ch == ')' and stack:
                start = stack.pop()
                pairs.append((start, i))
        for s, e in pairs:
            self.setFormat(s, 1, self.formats['paren'])
            self.setFormat(e, 1, self.formats['paren'])
        for pos in stack:
            self.setFormat(pos, 1, self.formats['unmatched'])
            
        # Highlight cross-sheet references and LN references with unique colors - case insensitive
        sheet_re = QRegularExpression(r"\bs\.(.*?)\.ln(\d+)\b", QRegularExpression.CaseInsensitiveOption)
        ln_re = QRegularExpression(r"\bln(\d+)\b", QRegularExpression.CaseInsensitiveOption)
        
        # First highlight sheet references
        it = sheet_re.globalMatch(text)
        while it.hasNext():
            m = it.next()
            sheet_name = m.captured(1)
            ln_num = int(m.captured(2))
            color = self.get_ln_color(ln_num)
            fmt = self._fmt(color)
            # Highlight the entire reference
            self.setFormat(m.capturedStart(), m.capturedLength(), fmt)
            # Add special formatting for sheet name
            sheet_fmt = self._fmt("#4DA6FF")  # Use operator color for sheet name
            sheet_start = m.capturedStart() + 2  # Skip "s."
            sheet_len = len(sheet_name)
            self.setFormat(sheet_start, sheet_len, sheet_fmt)
            
        # Then highlight regular LN references
        it = ln_re.globalMatch(text)
        while it.hasNext():
            m = it.next()
            ln_num = int(m.captured(1))
            color = self.get_ln_color(ln_num)
            fmt = self._fmt(color)
            self.setFormat(m.capturedStart(), m.capturedLength(), fmt)
            
        # Store the block data
        block = self.currentBlock()
        block.setUserData(LineData(block.blockNumber() + 1))

class ResultsHighlighter(BaseHighlighter):
    """Simple syntax highlighter for results panel, highlighting errors and important information"""
    
    def __init__(self, document):
        super().__init__(document)
        self.error_format = self._fmt("#FF5C5C", bold=True)
        
    def highlightBlock(self, text):
        # Highlight error text
        if "ERROR" in text or "TC ERROR" in text:
            self.setFormat(0, len(text), self.error_format)

class AutoCompleteDescriptionBox(QLabel):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowFlags(Qt.ToolTip | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint)
        self.setAttribute(Qt.WA_ShowWithoutActivating)
        self.setFocusPolicy(Qt.NoFocus)
        self.setWordWrap(True)
        self.setAlignment(Qt.AlignTop | Qt.AlignLeft)
        self.setMargin(8)
        self.setStyleSheet("""
            QLabel {
                background-color: #3a3a3c;
                color: #e0e0e0;
                border: 1px solid #4477ff;
                padding: 8px;
                font-family: 'Segoe UI';
                font-size: 11pt;
                line-height: 1.4em;
            }
        """)
        self.hide()

class AutoCompleteList(QListWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowFlags(Qt.ToolTip | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint)
        self.setAttribute(Qt.WA_ShowWithoutActivating)
        self.setFocusPolicy(Qt.NoFocus)
        self.setMouseTracking(True)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.setVerticalScrollBarPolicy(Qt.ScrollBarAsNeeded)  # Show scrollbar when needed
        self.setUniformItemSizes(True)
        self.setStyleSheet("""
            QListWidget {
                background-color: #2c2c2e;
                color: white;
                border: 1px solid #4477ff;
                selection-background-color: #4477ff;
                selection-color: white;
                padding: 2px;
                font-family: 'Courier New';
                font-size: 14pt;
            }
            QListWidget::item {
                padding: 4px 8px 4px 4px;
            }
            QListWidget::item:selected {
                background-color: #4477ff;
                margin-right: 2px;
            }
            QListWidget::item:hover {
                background-color: #3a3a3d;
                margin-right: 2px;
            }
            QScrollBar:vertical {
                background-color: #2c2c2e;
                width: 6px;
                border: none;
            }
            QScrollBar::handle:vertical {
                background-color: #4477ff;
                border-radius: 3px;
                min-height: 20px;
            }
            QScrollBar::handle:vertical:hover {
                background-color: #5588ff;
            }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
                border: none;
                background: none;
                height: 0px;
            }
            QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
                background: none;
            }
        """)
        
        # Connect selection change to update description
        self.currentRowChanged.connect(self.on_selection_changed)
        
        # Create description box
        self.description_box = AutoCompleteDescriptionBox(parent)
        
        # Function descriptions - keep them short and concise
        self.function_descriptions = {
            'sum': 'Adds numbers from a range of lines',
            'mean': 'Calculates average of numbers from a range',
            'median': 'Finds middle value in a range of numbers',
            'mode': 'Finds most frequently occurring value',
            'min': 'Finds smallest value in a range',
            'max': 'Finds largest value in a range',
            'count': 'Counts non-empty values in a range',
            'product': 'Multiplies all numbers in a range',
            'variance': 'Calculates variance of numbers in range',
            'stdev': 'Calculates standard deviation of range',
            'std': 'Calculates standard deviation of range',
            'range': 'Calculates max minus min of range',
            'geomean': 'Calculates geometric mean of range',
            'harmmean': 'Calculates harmonic mean of range',
            'sumsq': 'Calculates sum of squares in range',
            'perc5': 'Finds 5th percentile value in range',
            'perc95': 'Finds 95th percentile value in range',
            'meanfps': 'Calculates average with frame rate consideration',
            'sqrt': 'Calculates square root of a number',
            'sin': 'Calculates sine of an angle (radians)',
            'cos': 'Calculates cosine of an angle (radians)', 
            'tan': 'Calculates tangent of an angle (radians)',
            'asin': 'Calculates inverse sine (returns radians)',
            'acos': 'Calculates inverse cosine (returns radians)',
            'atan': 'Calculates inverse tangent (returns radians)',
            'sinh': 'Calculates hyperbolic sine',
            'cosh': 'Calculates hyperbolic cosine',
            'tanh': 'Calculates hyperbolic tangent',
            'asinh': 'Calculates inverse hyperbolic sine',
            'acosh': 'Calculates inverse hyperbolic cosine',
            'atanh': 'Calculates inverse hyperbolic tangent',
            'log': 'Calculates natural logarithm',
            'log10': 'Calculates base-10 logarithm',
            'log2': 'Calculates base-2 logarithm',
            'exp': 'Calculates e raised to the power of x',
            'pow': 'Raises first number to power of second',
            'ceil': 'Rounds number up to nearest integer',
            'floor': 'Rounds number down to nearest integer',
            'abs': 'Returns absolute value of number',
            'factorial': 'Calculates factorial of a number',
            'gcd': 'Finds greatest common divisor of two numbers',
            'lcm': 'Finds least common multiple of two numbers',
            'TC': 'Timecode calculation and conversion',
            'AR': 'Aspect ratio calculation for video dimensions',
            'D': 'Date arithmetic and business day calculations',
            'TR': 'Rounds number to specified decimal places',
            'truncate': 'Rounds number to specified decimal places',
            'pi': 'Mathematical constant π (3.14159...)',
            'e': 'Mathematical constant e (2.71828...)'
        }
        
        # Mode descriptions for different function contexts
        self.mode_descriptions = {
            'above': 'Uses all lines above current line',
            'below': 'Uses all lines below current line',
            'start range - end range': 'Uses lines from start to end (e.g., 1-5)',
            'line1,line2,line3': 'Uses specific line numbers (comma-separated)',
            'cg-above': 'Uses lines above in current group',
            'cg-below': 'Uses lines below in current group',
            'fps, timecode': 'Convert timecode to frames',
            'fps, frames': 'Convert frames to timecode',
            'fps, "timecode + timecode"': 'Add two timecodes together',
            'fps, "timecode - timecode"': 'Subtract one timecode from another',
            '1920x1080, ?x2000': 'Calculate width for given height',
            '1920x1080, 1280x?': 'Calculate height for given width',
            'original_width x original_height, target_width x ?': 'Calculate height for target width',
            'original_width x original_height, ? x target_height': 'Calculate width for target height',
            'date start range - date end range': 'Calculate days between two dates',
            'date start range W- date end range': 'Calculate business days between dates',
            'date start range + no. of days': 'Add days to a date',
            'date start range W+ no. of days': 'Add business days to a date',
            'value': 'Apply function to a single value',
            'expression': 'Apply function to result of expression',
            'value, decimals': 'First parameter is value, second is decimal places',
            'first_value, second_value': 'Function takes two parameters',
            '23.976': 'Standard film frame rate',
            '24': 'Cinema frame rate',
            '25': 'PAL video frame rate',
            '29.97': 'NTSC drop-frame rate',
            '30': 'NTSC non-drop frame rate',
            '50': 'PAL progressive frame rate',
            '59.94': 'NTSC progressive frame rate',
            '60': 'High frame rate'
        }

    def on_selection_changed(self, current_row):
        """Update description box when selection changes"""
        if current_row >= 0 and current_row < self.count():
            item = self.item(current_row)
            if item:
                item_text = item.text()
                description = self.get_description_for_item(item_text)
                if description:
                    self.description_box.setText(description)
                    self.show_description_box()
                else:
                    self.description_box.hide()
        else:
            self.description_box.hide()

    def get_description_for_item(self, item_text):
        """Get description for the given item"""
        # Check if it's a function
        if item_text in self.function_descriptions:
            return self.function_descriptions[item_text]
        
        # Check if it's a mode/parameter option
        if item_text in self.mode_descriptions:
            return self.mode_descriptions[item_text]
        
        # Handle currency completions (end with " to ")
        if item_text.endswith(' to '):
            return f"Convert from {item_text.replace(' to ', '')} to another currency"
        
        return None

    def show_description_box(self):
        """Show the description box positioned to the right of the completion list"""
        if not self.isVisible():
            return
            
        # Position description box to the right of the completion list
        list_rect = self.geometry()
        desc_x = list_rect.right() + 10  # 10px gap
        desc_y = list_rect.top()
        
        # Set a reasonable width for the description box
        desc_width = 250
        desc_height = 80
        
        # Get screen geometry to ensure description stays on screen
        screen = QApplication.screenAt(self.mapToGlobal(QPoint(0, 0)))
        if not screen:
            screen = QApplication.primaryScreen()
        screen_geom = screen.geometry()
        
        # Adjust position if it would go off screen
        if desc_x + desc_width > screen_geom.right():
            # Position to the left of the completion list instead
            desc_x = list_rect.left() - desc_width - 10
        
        if desc_y + desc_height > screen_geom.bottom():
            desc_y = screen_geom.bottom() - desc_height
            
        self.description_box.setGeometry(desc_x, desc_y, desc_width, desc_height)
        self.description_box.show()
        self.description_box.raise_()

    def hide(self):
        """Hide both the completion list and description box"""
        super().hide()
        self.description_box.hide()

    def handle_key_event(self, key):
        if key == Qt.Key_Up:
            current = self.currentRow()
            next_row = self.count() - 1 if current <= 0 else current - 1
            self.setCurrentRow(next_row)
            return True
        elif key == Qt.Key_Down:
            current = self.currentRow()
            next_row = 0 if current >= self.count() - 1 else current + 1
            self.setCurrentRow(next_row)
            return True
        elif key in (Qt.Key_Enter, Qt.Key_Return, Qt.Key_Tab):
            if self.parent():
                self.parent().complete_text()
            return True
        elif key == Qt.Key_Escape:
            self.hide()
            return True
        return False

class KeyEventFilter(QObject):
    """Event filter to intercept key events before Qt's internal handling"""
    def __init__(self, editor):
        super().__init__()
        self.editor = editor
    
    def eventFilter(self, obj, event):
        if event.type() == QEvent.KeyPress and obj == self.editor:
            ctrl = event.modifiers() & Qt.ControlModifier
            alt = event.modifiers() & Qt.AltModifier
            k = event.key()

            # Debug: Log all key events to see what's being filtered
            if k in (Qt.Key_Delete, Qt.Key_Backspace) or (ctrl and k == Qt.Key_A):
                cursor = self.editor.textCursor()
                has_selection = cursor.hasSelection()
                print(f"DEBUG: KeyEventFilter - Key: {k}, Modifiers: {event.modifiers()}, Has selection: {has_selection}")

            # Check if we have a selection when Ctrl key events come in
            cursor = self.editor.textCursor()
            has_selection = cursor.hasSelection()

            # Preserve selection for Ctrl key
            if has_selection and k == 16777249:  # This is the Ctrl key
                return True

            # Handle Ctrl+C with selection directly here to prevent Qt from clearing selection
            if has_selection and ctrl and k == Qt.Key_C:
                selected_text = cursor.selectedText()
                if selected_text:
                    # Convert Qt's special line breaks to regular line breaks
                    text = selected_text.replace('\u2029', '\n').replace('\u2028', '\n')
                    clipboard = QApplication.clipboard()
                    clipboard.setText(text)
                    return True  # Event handled, don't pass to Qt

            # For Ctrl+arrow keys with an existing selection, preserve selection
            # EXCEPT for Ctrl+Up and Ctrl+Down which have special functions in keyPressEvent
            elif has_selection and ctrl and k in (Qt.Key_Left, Qt.Key_Right):
                return True  # Block these arrow keys to preserve selection

        # For all other events, let keyPressEvent handle them
        return False

class EditorAutoCompletionMixin:
    """Handles all auto-completion functionality for the formula editor"""
    
    def setup_autocompletion(self):
        """Setup auto-completion data structures and completions"""
        # Basic functions and commands - simplified to just show function names
        self.base_completions = {
            'TR': 'TR',
            'truncate': 'truncate',
            'sqrt': 'sqrt',
            'sin': 'sin',
            'cos': 'cos',
            'tan': 'tan',
            'asin': 'asin',
            'acos': 'acos',
            'atan': 'atan',
            'sinh': 'sinh',
            'cosh': 'cosh',
            'tanh': 'tanh',
            'asinh': 'asinh',
            'acosh': 'acosh',
            'atanh': 'atanh',
            'log': 'log',
            'log10': 'log10',
            'log2': 'log2',
            'exp': 'exp',
            'pow': 'pow',
            'ceil': 'ceil',
            'floor': 'floor',
            'abs': 'abs',
            'factorial': 'factorial',
            'gcd': 'gcd',
            'lcm': 'lcm',
            'TC': 'TC',
            'AR': 'AR',
            'D': 'D',
            'pi': 'pi',
            'e': 'e',
            # Statistical functions - just show function names
            'sum': 'sum',
            'mean': 'mean',
            'meanfps': 'meanfps',
            'median': 'median',
            'mode': 'mode',
            'min': 'min',
            'max': 'max',
            'range': 'range',
            'count': 'count',
            'product': 'product',
            'variance': 'variance',
            'stdev': 'stdev',
            'std': 'std',
            'geomean': 'geomean',
            'harmmean': 'harmmean',
            'sumsq': 'sumsq',
            'perc5': 'perc5',
            'perc95': 'perc95'
        }
        
        # Parameter options for different function types
        self.statistical_range_options = [
            'above',
            'below', 
            'start range - end range',
            'line1,line2,line3',
            'cg-above',
            'cg-below'
        ]
        
        self.tc_options = [
            'fps, timecode',
            'fps, frames', 
            'fps, "timecode + timecode"',
            'fps, "timecode - timecode"'
        ]
        
        self.ar_options = [
            '1920x1080, ?x2000',
            '1920x1080, 1280x?',
            'original_width x original_height, target_width x ?',
            'original_width x original_height, ? x target_height'
        ]
        
        self.date_options = [
            'date start range - date end range',
            'date start range W- date end range',
            'date start range + no. of days',
            'date start range W+ no. of days'
        ]
        
        self.basic_math_options = [
            'value',
            'expression'
        ]
        
        self.two_param_options = [
            'value, decimals',
            'first_value, second_value'
        ]
        
        # Functions that use statistical range options
        self.statistical_functions = {
            'sum', 'mean', 'median', 'mode', 'min', 'max', 'range', 'count', 
            'product', 'variance', 'stdev', 'std', 'geomean', 'harmmean', 
            'sumsq', 'perc5', 'perc95'
        }

    def get_word_under_cursor(self):
        """Extract word at cursor position with special handling for currency completions"""
        cursor = self.textCursor()
        text = cursor.block().text()
        pos = cursor.positionInBlock()
        
        # For currency completion, we might need to handle multi-word currencies
        # like "canadian dollars", so let's be more flexible
        
        # First check if we're in a currency context
        line_text = text
        cursor_pos = pos
        
        # Check for currency conversion patterns
        currency_to_pattern = r'([\d.]+)\s+(.+?)\s+to\s+(\w*)$'
        currency_source_pattern = r'([\d.]+)\s+(\w+)$'
        
        # If we're after "to", return the partial target currency
        to_match = re.search(currency_to_pattern, line_text[:cursor_pos], re.IGNORECASE)
        if to_match:
            return to_match.group(3)
        
        # If we're typing a source currency, return the current word
        source_match = re.search(currency_source_pattern, line_text[:cursor_pos], re.IGNORECASE)
        if source_match:
            return source_match.group(2)
        
        # Default word finding for regular completions
        left = pos - 1
        while left >= 0 and (text[left].isalnum() or text[left] == '-'):
            left -= 1
        left += 1
        
        right = pos
        while right < len(text) and (text[right].isalnum() or text[right] == '-'):
            right += 1
            
        # Extract the word
        word = text[left:right].strip()
        
        # If we have hyphens before the word, include them
        hyphens = ''
        while left > 0 and text[left - 1] == '-':
            hyphens = '-' + hyphens
            left -= 1
            
        return hyphens + word

    def get_completions(self, prefix):
        """Generate completion suggestions based on context and prefix"""
        completions = []
        
        # Skip completions for LN references
        if prefix.lower().startswith('ln'):
            return []
        
        # Get the current line text to check for different contexts
        cursor = self.textCursor()
        line_text = cursor.block().text()
        cursor_pos = cursor.positionInBlock()
        
        # Check if we're inside function parentheses
        text_before_cursor = line_text[:cursor_pos]
        
        # Look for function pattern: function_name(
        function_pattern = r'(\w+)\(\s*([^)]*?)$'
        function_match = re.search(function_pattern, text_before_cursor)
        
        if function_match:
            function_name = function_match.group(1).lower()
            params_so_far = function_match.group(2)
            
            # We're inside a function - show parameter options
            if function_name in self.statistical_functions:
                # For statistical functions, show range options
                if function_name == 'meanfps':
                    # Special case for meanfps - needs fps parameter first
                    if ',' not in params_so_far:
                        # First parameter (fps)
                        fps_options = ['23.976', '24', '25', '29.97', '30', '50', '59.94', '60']
                        for fps in fps_options:
                            if fps.startswith(prefix.lower()):
                                completions.append(fps)
                    else:
                        # Second parameter (range options)
                        for option in self.statistical_range_options:
                            if not prefix or option.lower().startswith(prefix.lower()):
                                completions.append(option)
                else:
                    # Regular statistical functions - show range options
                    for option in self.statistical_range_options:
                        if not prefix or option.lower().startswith(prefix.lower()):
                            completions.append(option)
                            
            elif function_name == 'tc':
                # TC function options
                for option in self.tc_options:
                    if not prefix or prefix.lower() in option.lower():
                        completions.append(option)
                        
            elif function_name == 'ar':
                # AR function options  
                for option in self.ar_options:
                    if not prefix or prefix.lower() in option.lower():
                        completions.append(option)
                        
            elif function_name == 'd':
                # Date function options
                for option in self.date_options:
                    if not prefix or prefix.lower() in option.lower():
                        completions.append(option)
                        
            elif function_name in ['tr', 'truncate']:
                # TR/truncate function options
                for option in self.two_param_options:
                    if 'decimal' in option.lower() and (not prefix or prefix.lower() in option.lower()):
                        completions.append(option)
                        
            elif function_name in ['pow', 'gcd', 'lcm']:
                # Two parameter functions
                for option in self.two_param_options:
                    if 'value' in option.lower() and (not prefix or prefix.lower() in option.lower()):
                        completions.append(option)
                        
            else:
                # Basic math functions
                for option in self.basic_math_options:
                    if not prefix or prefix.lower() in option.lower():
                        completions.append(option)
            
            return sorted(completions)
        
        # Check if we're in a currency conversion context
        # Pattern: number + currency + "to" + partial_currency
        currency_pattern = r'([\d.]+)\s+(\w+)\s+to\s+(\w*)$'
        match = re.search(currency_pattern, text_before_cursor, re.IGNORECASE)
        
        if match:
            # We're completing the target currency
            partial_currency = match.group(3).lower()
            # Get all currency names that start with the partial input
            for currency_name in CURRENCY_ABBR.keys():
                if currency_name.startswith(partial_currency):
                    completions.append(currency_name)
            return sorted(completions)
        
        # Check if we're typing a source currency after a number
        # Pattern: number + partial_currency (but not followed by "to")
        source_currency_pattern = r'([\d.]+)\s+(\w+)$'
        match = re.search(source_currency_pattern, text_before_cursor, re.IGNORECASE)
        
        if match:
            # Check if the partial word could be a currency
            partial_currency = match.group(2).lower()
            # Only suggest currencies if the partial input matches currency names
            currency_matches = []
            for currency_name in CURRENCY_ABBR.keys():
                if currency_name.startswith(partial_currency):
                    currency_matches.append(currency_name + " to ")
            
            if currency_matches:
                return sorted(currency_matches)
        
        # Default: show function names
        for key, value in self.base_completions.items():
            if key.lower().startswith(prefix.lower()):
                completions.append(value)

        return sorted(completions)

    def complete_text(self, item=None):
        """Apply selected completion to the text"""
        if item is None:
            item = self.completion_list.currentItem()
        if item is None:
            return

        completion_text = item.text()
        cursor = self.textCursor()
        
        # Get current line context
        line_text = cursor.block().text()
        cursor_pos = cursor.positionInBlock()
        text_before_cursor = line_text[:cursor_pos]
        
        # Check if we're completing inside function parentheses
        function_pattern = r'(\w+)\(\s*([^)]*?)$'
        function_match = re.search(function_pattern, text_before_cursor)
        
        if function_match:
            # We're inside a function - completing parameters
            cursor.select(QTextCursor.WordUnderCursor)
            cursor.removeSelectedText()
            cursor.insertText(completion_text + ')')  # Add closing parenthesis
            # Move cursor back one position to be just before the closing parenthesis
            cursor.movePosition(QTextCursor.Left, QTextCursor.MoveAnchor, 1)
        else:
            # We're completing a function name or other top-level completion
            
            # For --- commands, handle the replacement specially
            if completion_text.startswith('---'):
                # Get the current line text
                cursor.movePosition(QTextCursor.StartOfLine)
                cursor.movePosition(QTextCursor.EndOfLine, QTextCursor.KeepAnchor)
                line_text = cursor.selectedText()
                
                # Find where the dashes start
                dash_start = line_text.find('-')
                if dash_start >= 0:
                    # Move to the start of the dashes
                    cursor.movePosition(QTextCursor.StartOfLine)
                    cursor.movePosition(QTextCursor.Right, QTextCursor.MoveAnchor, dash_start)
                    # Select from there to end of line
                    cursor.movePosition(QTextCursor.EndOfLine, QTextCursor.KeepAnchor)
                    # Count the dashes at current position
                    dash_count = 0
                    while dash_count < len(line_text) - dash_start and line_text[dash_start + dash_count] == '-':
                        dash_count += 1
                    
                    # If we have exactly 3 dashes, preserve them and add the function
                    if dash_count == 3:
                        cursor.movePosition(QTextCursor.StartOfLine)
                        cursor.movePosition(QTextCursor.Right, QTextCursor.MoveAnchor, dash_start + 3)
                        cursor.movePosition(QTextCursor.EndOfLine, QTextCursor.KeepAnchor)
                        cursor.removeSelectedText()
                        cursor.insertText(completion_text[3:])
                    else:
                        # Otherwise replace everything with the full completion
                        cursor.movePosition(QTextCursor.StartOfLine)
                        cursor.movePosition(QTextCursor.Right, QTextCursor.MoveAnchor, dash_start)
                        cursor.movePosition(QTextCursor.EndOfLine, QTextCursor.KeepAnchor)
                        cursor.removeSelectedText()
                        cursor.insertText(completion_text)
                else:
                    # If no dashes found, insert the full completion
                    cursor.removeSelectedText()
                    cursor.insertText(completion_text)
            else:
                # Handle regular completions
                cursor.select(QTextCursor.WordUnderCursor)
                cursor.removeSelectedText()
                
                # Check if this is a function name that needs parentheses
                is_function = (completion_text in self.base_completions and 
                             completion_text not in ['pi', 'e'] and  # Constants don't need parentheses
                             not completion_text.endswith(' to '))  # Currency completions
                
                if is_function:
                    # Add parentheses and position cursor inside
                    cursor.insertText(completion_text + '(')
                    # Trigger completion again to show parameter options after a short delay
                    QTimer.singleShot(10, self.show_completion_popup)
                else:
                    cursor.insertText(completion_text)
            
        self.setTextCursor(cursor)
        
        # Only hide completion list if we're not expecting parameter completion
        if not (completion_text in self.base_completions and 
               completion_text not in ['pi', 'e'] and 
               not completion_text.endswith(' to ')):
            self.completion_list.hide()

    def show_completion_popup(self):
        """Display completion popup with suggestions"""
        text_cursor = self.textCursor()
        current_word = self.get_word_under_cursor()
        
        # Check if we're inside function parentheses even if there's no current word
        line_text = text_cursor.block().text()
        cursor_pos = text_cursor.positionInBlock()
        text_before_cursor = line_text[:cursor_pos]
        
        # Look for function pattern: function_name(
        function_pattern = r'(\w+)\(\s*([^)]*?)$'
        function_match = re.search(function_pattern, text_before_cursor)
        
        # If we're inside a function, allow empty current_word for parameter completion
        inside_function = function_match is not None
        
        # Don't show completion for pure numbers (but allow empty strings inside functions)
        if (not inside_function and not current_word) or (current_word and current_word.isdigit()):
            self.completion_list.hide()
            return

        # Get completions - use empty string if no current_word but we're inside a function
        search_prefix = current_word if current_word else ""
        completions = self.get_completions(search_prefix)
        if not completions:
            self.completion_list.hide()
            return

        # Update completion list
        self.completion_list.clear()
        self.completion_list.addItems(completions)

        # Calculate popup position
        cursor_rect = self.cursorRect()
        screen_pos = self.mapToGlobal(cursor_rect.bottomLeft())
        
        # Get the screen that contains the editor
        window = self.window()
        window_center = window.geometry().center()
        screen = QApplication.screenAt(window_center)
        if not screen:
            screen = QApplication.primaryScreen()
        
        screen_geom = screen.geometry()

        # Calculate size based on content
        fm = self.completion_list.fontMetrics()
        max_width = max(fm.horizontalAdvance(item) for item in completions) + 40  # Add padding
        width = max(200, max_width)  # Ensure minimum width
        
        # Always size popup to show 5 items - simpler approach
        item_height = 30
        visible_items = 5
        padding = 4
        height = (visible_items * item_height) + padding  # Always 154px
        
        # Only show scrollbar if there are more than 5 items
        if self.completion_list.count() > visible_items:
            self.completion_list.setVerticalScrollBarPolicy(Qt.ScrollBarAsNeeded)
        else:
            self.completion_list.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        
        # Adjust position to be just below the text
        screen_pos.setY(screen_pos.y() + 5)

        # Ensure popup stays within screen bounds
        if screen_pos.x() + width > screen_geom.right():
            screen_pos.setX(screen_geom.right() - width)
        if screen_pos.y() + height > screen_geom.bottom():
            screen_pos.setY(cursor_rect.top() - height - 5)

        # Set size and position
        self.completion_list.setFixedSize(width, height)
        self.completion_list.move(screen_pos)

        # Show and select first item
        self.completion_list.show()
        self.completion_list.setCurrentRow(0)
        self.completion_list.raise_()  # Ensure popup stays on top
        
        # Trigger description box for the first item
        if completions:
            self.completion_list.on_selection_changed(0)

class EditorPerformanceMonitoringMixin:
    """Handles all performance monitoring and debugging functionality for the formula editor"""
    
    def _log_perf(self, method_name, start_time=None):
        """Log performance measurements"""
        if not self._debug_enabled:
            return
            
        current_time = time.time() * 1000
        if start_time is None:
            # Starting measurement
            self._call_stack.append((method_name, current_time))
            return current_time
        else:
            # Ending measurement
            duration = current_time - start_time
            if duration > 10:  # Only log operations taking more than 10ms
                cursor_line = self.textCursor().blockNumber()
                log_entry = f"[{current_time:.0f}] {method_name}: {duration:.1f}ms (line {cursor_line})"
                self._perf_log.append(log_entry)
                print(log_entry)  # Real-time console output
                
                # Keep only last 50 entries
                if len(self._perf_log) > 50:
                    self._perf_log = self._perf_log[-50:]
            
            # Remove from call stack
            if self._call_stack and self._call_stack[-1][0] == method_name:
                self._call_stack.pop()

    def _check_scroll_sync_issue(self):
        """Check if scroll positions are mismatched"""
        if not self._debug_enabled or not hasattr(self.parent, 'results'):
            return
            
        editor_scroll = self.verticalScrollBar().value()
        results_scroll = self.parent.results.verticalScrollBar().value()
        
        editor_max = self.verticalScrollBar().maximum()
        results_max = self.parent.results.verticalScrollBar().maximum()
        
        if editor_max > 0 and results_max > 0:
            editor_ratio = editor_scroll / editor_max
            results_ratio = results_scroll / results_max
            
            if abs(editor_ratio - results_ratio) > 0.05:  # 5% difference
                print(f"SCROLL SYNC ISSUE: Editor {editor_scroll}/{editor_max} ({editor_ratio:.2f}) vs Results {results_scroll}/{results_max} ({results_ratio:.2f})")

    def print_perf_summary(self):
        """Print recent performance log to console"""
        if not self._debug_enabled:
            return
            
        print("\n=== PERFORMANCE LOG (Last 10 entries) ===")
        for entry in self._perf_log[-10:]:
            print(entry)
        print("==========================================\n")

class EditorCrossSheetMixin:
    """Handles all cross-sheet reference functionality for the formula editor"""
    
    def get_sheet_value(self, sheet_name, ln_number):
        """Get a value from a specific sheet by name and line number"""
        calculator = self.get_calculator()
        if not calculator:
            return None
            
        # Find the sheet by name (case insensitive)
        for i in range(calculator.tabs.count()):
            if calculator.tabs.tabText(i).lower() == sheet_name.lower():
                sheet = calculator.tabs.widget(i)
                if hasattr(sheet, 'editor'):
                    # First try to find the line by its ID
                    if ln_number in sheet.editor.ln_value_map:
                        return sheet.editor.ln_value_map[ln_number]
                    
                    # If not found by ID, try to find by line number
                    doc = sheet.editor.document()
                    if ln_number <= doc.blockCount():
                        block = doc.findBlockByNumber(ln_number - 1)  # Convert to 0-based index
                        if block.isValid():
                            user_data = block.userData()
                            if isinstance(user_data, LineData):
                                line_id = user_data.id
                                if line_id in sheet.editor.ln_value_map:
                                    return sheet.editor.ln_value_map[line_id]
        return None

    def get_numeric_value(self, value):
        """Extract numeric value from a result, stripping any units or formatting"""
        if isinstance(value, (int, float)):
            return value
        if isinstance(value, dict) and 'value' in value:
            return value['value']
        if isinstance(value, str):
            # Try to extract first number from string
            match = re.match(r'[-+]?\d*\.?\d+', value)
            if match:
                return float(match.group())
        return value

    def process_ln_refs(self, expr):
        """Replace LN references and cross-sheet references with their values - Stage 2 Optimized"""
        # Stage 2 Optimization: Check cache first
        expr_hash = hash(expr)
        if expr_hash in self._ln_reference_cache:
            return self._ln_reference_cache[expr_hash]
        
        original_expr = expr
        
        # Stage 2 Optimization: Single-pass normalization using pre-compiled patterns
        expr = self._ln_normalization_pattern.sub(lambda m: f"S.{m.group(1)}.LN", expr)
        expr = self._ln_case_pattern.sub(lambda m: f"LN{m.group(1)}", expr)
        
        # Stage 2 Optimization: Single-pass replacement using efficient callback
        def repl(m):
            # Check if this is a cross-sheet reference
            if m.group(1) and m.group(2):  # We have a sheet reference
                sheet_name = m.group(2)
                ln = int(m.group(3))
                v = self.get_sheet_value(sheet_name, ln)
                if v is not None:
                    # Extract numeric value if needed
                    v = self.get_numeric_value(v)
                    return str(v)
                return "0"
            else:  # Regular LN reference
                ln = int(m.group(4))
                if ln in self.ln_value_map:
                    v = self.ln_value_map[ln]
                    if v is not None:
                        # Extract numeric value if needed
                        v = self.get_numeric_value(v)
                        return str(v)
                return "0"
        
        # Stage 2 Optimization: Single-pass replacement instead of loop
        # Check if we need to process any LN references at all
        if 'LN' in expr:
            prev_expr = None
            iteration_count = 0
            max_iterations = 10  # Safety limit to prevent infinite loops
            
            while prev_expr != expr and iteration_count < max_iterations:
                prev_expr = expr
                expr = self._ln_combined_pattern.sub(repl, expr)
                iteration_count += 1
                
                # Early exit if no more LN references to process
                if 'LN' not in expr:
                    break
        
        # Stage 2 Optimization: Cache the result for future use
        self._ln_reference_cache[expr_hash] = expr
        
        # Limit cache size to prevent memory issues
        if len(self._ln_reference_cache) > 1000:
            # Remove oldest entries (simple FIFO)
            items = list(self._ln_reference_cache.items())
            self._ln_reference_cache = dict(items[-500:])  # Keep last 500 entries
        
        return expr

    def build_cross_sheet_cache(self):
        """Build cache for fast cross-sheet lookups"""
        calculator = self.get_calculator()
        if not calculator:
            return
            
        self._cross_sheet_cache.clear()
        
        # Build cache for all sheets
        for i in range(calculator.tabs.count()):
            sheet = calculator.tabs.widget(i)
            if sheet != self.parent and hasattr(sheet, 'editor'):
                sheet_name = calculator.tabs.tabText(i).lower()
                sheet_cache = {}
                
                doc = sheet.editor.document()
                for j in range(doc.blockCount()):
                    blk = doc.findBlockByNumber(j)
                    user_data = blk.userData()
                    if isinstance(user_data, LineData):
                        sheet_cache[user_data.id] = j
                        
                self._cross_sheet_cache[sheet_name] = sheet_cache

    def clear_highlighted_sheets_only(self):
        """Clear highlights only from sheets that were previously highlighted"""
        calculator = self.get_calculator()
        if not calculator:
            return
            
        # Only clear sheets that we know have highlights
        for sheet in self._highlighted_sheets:
            if hasattr(sheet, 'editor'):
                sheet.editor.setExtraSelections([])
                if hasattr(sheet, 'results'):
                    sheet.results.setExtraSelections([])
        
        # Clear the set of highlighted sheets
        self._highlighted_sheets.clear()

class EditorTextSelectionMixin:
    """Handles all text selection and navigation functionality for the formula editor"""
    
    def select_number_token(self, forward=True):
        """Select the next/previous number token from current cursor position"""
        cursor = self.textCursor()
        block = cursor.block()
        
        # If we have a selection, use its start or end as the position
        if cursor.hasSelection():
            pos = cursor.selectionEnd() - block.position() if forward else cursor.selectionStart() - block.position()
            # Move past the current selection
            if not forward:
                pos -= 1
        else:
            pos = cursor.positionInBlock()
            
        start_block = block
        
        while True:
            text = block.text()
            numbers = list(re.finditer(r'\b\d+(?:\.\d+)?\b', text))
            
            if forward:
                next_num = next((n for n in numbers if n.start() >= pos), None)
                if next_num:
                    cursor.setPosition(block.position() + next_num.start())
                    cursor.setPosition(block.position() + next_num.end(), QTextCursor.KeepAnchor)
                    self.setTextCursor(cursor)
                    return
                block = block.next()
                if not block.isValid():
                    block = self.document().firstBlock()
                pos = 0
            else:
                prev_nums = [n for n in numbers if n.end() <= pos]
                if prev_nums:
                    next_num = prev_nums[-1]
                    cursor.setPosition(block.position() + next_num.start())
                    cursor.setPosition(block.position() + next_num.end(), QTextCursor.KeepAnchor)
                    self.setTextCursor(cursor)
                    return
                block = block.previous()
                if not block.isValid():
                    block = self.document().lastBlock()
                pos = len(block.text())
            
            if block == start_block:
                return

    def expand_selection_with_parens(self):
        """Expand selection progressively based on parentheses levels"""
        cursor = self.textCursor()
        text = cursor.block().text()
        block_pos = cursor.block().position()
        
        # Get current cursor position or selection bounds (relative to line start)
        if cursor.hasSelection():
            sel_start = cursor.selectionStart() - block_pos
            sel_end = cursor.selectionEnd() - block_pos
        else:
            pos = cursor.positionInBlock()
            sel_start = sel_end = pos
        
        # Find all matching parentheses pairs
        stack = []
        pairs = []
        for i, ch in enumerate(text):
            if ch == '(':
                stack.append(i)
            elif ch == ')' and stack:
                start = stack.pop()
                pairs.append((start, i))
        
        # Sort pairs by nesting level (innermost first)
        pairs.sort(key=lambda p: p[1] - p[0])
        
        # Find all parentheses levels that contain the cursor/selection
        containing_pairs = []
        for start, end in pairs:
            # Check if this pair contains the current cursor/selection
            if start <= sel_start and end >= sel_end:
                containing_pairs.append((start, end))
        
        # Sort containing pairs by size (innermost first)
        containing_pairs.sort(key=lambda p: p[1] - p[0])
        
        # If we have no selection, start with the innermost parentheses content
        if not cursor.hasSelection():
            if containing_pairs:
                start, end = containing_pairs[0]
                # Select content inside parentheses (excluding the parentheses)
                cursor.setPosition(block_pos + start + 1)
                cursor.setPosition(block_pos + end, QTextCursor.KeepAnchor)
                self.setTextCursor(cursor)
                return
            else:
                # No parentheses, select entire line
                self.select_entire_line()
                return
        
        # We have a selection - find the next expansion level
        for i, (start, end) in enumerate(containing_pairs):
            # Check if we're selecting the content inside these parentheses (excluding parens)
            if sel_start == start + 1 and sel_end == end:
                # Expand to include the parentheses themselves
                cursor.setPosition(block_pos + start)
                cursor.setPosition(block_pos + end + 1, QTextCursor.KeepAnchor)
                self.setTextCursor(cursor)
                return
            
            # Check if we're selecting including these parentheses
            elif sel_start == start and sel_end == end + 1:
                # Find the next outer level
                if i + 1 < len(containing_pairs):
                    # Expand to content of next outer level
                    outer_start, outer_end = containing_pairs[i + 1]
                    cursor.setPosition(block_pos + outer_start + 1)
                    cursor.setPosition(block_pos + outer_end, QTextCursor.KeepAnchor)
                    self.setTextCursor(cursor)
                    return
                else:
                    # No outer level found, select entire line
                    self.select_entire_line()
                    return
        
        # If current selection doesn't match any parentheses pattern exactly,
        # find the smallest parentheses pair that would be the next logical expansion
        for start, end in containing_pairs:
            # If the selection is smaller than this parentheses content
            if sel_start >= start + 1 and sel_end <= end:
                # Select content inside this pair
                cursor.setPosition(block_pos + start + 1)
                cursor.setPosition(block_pos + end, QTextCursor.KeepAnchor)
                self.setTextCursor(cursor)
                return
        
        # Fallback: select entire line
        self.select_entire_line()

    def find_arithmetic_expression(self, text, pos):
        """Find the boundaries of an arithmetic expression at the given position"""
        # Find boundaries of numbers and operators
        tokens = list(re.finditer(r'\b\d+(?:\.\d+)?\b|[-+*/^]', text))
        for i, token in enumerate(tokens):
            if token.start() <= pos <= token.end():
                # Found token containing cursor, now expand to full expression
                start = end = i
                while start > 0 and tokens[start-1].group() in '+-*/^':
                    start -= 1
                while end < len(tokens)-1 and tokens[end+1].group() in '+-*/^':
                    end += 1
                return tokens[start].start(), tokens[end].end()
        return None

    def select_entire_line(self):
        """Select the entire current line, excluding the newline character"""
        cursor = self.textCursor()
        cursor.movePosition(QTextCursor.StartOfLine)
        cursor.movePosition(QTextCursor.EndOfLine, QTextCursor.KeepAnchor)
        self.setTextCursor(cursor)

    def select_nearest_word_or_number(self):
        """Select the nearest word or number from the cursor position"""
        cursor = self.textCursor()
        text = cursor.block().text()
        pos = cursor.positionInBlock()
        
        # Define what constitutes a word/number character
        def is_word_char(c):
            return c.isalnum() or c in '.-_'
        
        # Find the boundaries of the nearest word/number
        left = pos
        right = pos
        
        # If we're already on a word character, expand from current position
        if pos < len(text) and is_word_char(text[pos]):
            # Expand right
            while right < len(text) and is_word_char(text[right]):
                right += 1
            # Expand left
            while left > 0 and is_word_char(text[left - 1]):
                left -= 1
        else:
            # Look for the nearest word/number
            left_dist = right_dist = float('inf')
            
            # Look left
            temp_left = pos - 1
            while temp_left >= 0:
                if is_word_char(text[temp_left]):
                    left_dist = pos - temp_left
                    break
                temp_left -= 1
            
            # Look right
            temp_right = pos
            while temp_right < len(text):
                if is_word_char(text[temp_right]):
                    right_dist = temp_right - pos
                    break
                temp_right += 1
            
            # Choose the nearest word/number
            if left_dist <= right_dist and left_dist != float('inf'):
                # Word to the left is closer
                right = pos
                while temp_left >= 0 and is_word_char(text[temp_left]):
                    temp_left -= 1
                left = temp_left + 1
                while right < len(text) and is_word_char(text[right]):
                    right += 1
            elif right_dist != float('inf'):
                # Word to the right is closer
                left = temp_right
                while right < len(text) and is_word_char(text[right]):
                    right += 1
        
        # Set the selection
        if left != right:
            cursor.setPosition(cursor.block().position() + left)
            cursor.setPosition(cursor.block().position() + right, QTextCursor.KeepAnchor)
            self.setTextCursor(cursor)
            return True
        return False

    def get_selected_text(self):
        """Get selected text with proper line breaks"""
        cursor = self.textCursor()
        if not cursor.hasSelection():
            return ""
            
        # Get the raw text and normalize line endings
        text = cursor.selectedText()
        return text.replace('\u2029', '\n').replace('\u2028', '\n')

class EditorLineManagementMixin:
    """Handles all line management and highlighting functionality for the formula editor"""
    
    def assign_stable_ids(self):
        doc = self.document()
        for i in range(doc.blockCount()):
            blk = doc.findBlockByNumber(i)
            if not isinstance(blk.userData(), LineData):
                blk.setUserData(LineData(self.next_line_id))
                self.next_line_id += 1

    def reassign_line_ids(self):
        """Assign sequential, stable IDs from top to bottom."""
        doc = self.document()
        for i in range(doc.blockCount()):
            blk = doc.findBlockByNumber(i)
            blk.setUserData(LineData(i + 1))  # IDs start from 1
        self.next_line_id = doc.blockCount() + 1

    def highlight_expression(self, block, start, end):
        """Highlight the expression with a light background color"""
        # Create a text cursor for the selection
        cursor = QTextCursor(block)
        cursor.setPosition(block.position() + start)
        cursor.setPosition(block.position() + end, QTextCursor.KeepAnchor)
        
        # Create and store the selection
        selection = QTextEdit.ExtraSelection()
        selection.format.setBackground(QColor(95, 95, 96))  # Lighter highlight color for better visibility
        selection.cursor = cursor
        
        # Store the current highlight
        self.current_highlight = selection
        
        # Refresh the current line highlight to include this highlight
        self.highlightCurrentLine()

    def clear_expression_highlight(self):
        """Clear the expression highlight"""
        if self.current_highlight:
            self.current_highlight = None
            self.highlightCurrentLine()  # Refresh highlights without the operator highlight

    def highlightCurrentLine(self):
        """Highlight the current line and maintain any operator highlights"""
        start_time = self._log_perf("highlightCurrentLine")
        
        selections = []
        results_selections = []
        
        # Get LN references from current line first for early exit
        current_line_text = self.textCursor().block().text()
        ln_matches = list(re.finditer(r'\b(?:s\.(.*?)\.)?ln(\d+)\b', current_line_text, re.IGNORECASE))
        
        # Early exit if no LN references - just do basic highlighting
        if not ln_matches:
            # Clear any existing cross-sheet highlights efficiently
            clear_start = self._log_perf("clear_highlighted_sheets_only")
            self.clear_highlighted_sheets_only()
            self._log_perf("clear_highlighted_sheets_only", clear_start)
            
            # Basic current line highlight
            sel = QTextEdit.ExtraSelection()
            sel.format.setBackground(QColor(65, 65, 66))
            sel.format.setProperty(QTextCharFormat.FullWidthSelection, True)
            sel.cursor = self.textCursor()
            if not sel.cursor.hasSelection():
                sel.cursor.clearSelection()
            else:
                sel.cursor = QTextCursor(sel.cursor.block())
            selections.append(sel)
            
            # Add operator highlight if it exists
            if self.current_highlight:
                selections.append(self.current_highlight)
            
            # Basic result line highlight
            if hasattr(self.parent, 'results'):
                block_number = self.textCursor().blockNumber()
                results_block = self.parent.results.document().findBlockByNumber(block_number)
                if results_block.isValid():
                    results_cursor = QTextCursor(results_block)
                    sel_result = QTextEdit.ExtraSelection()
                    sel_result.format.setBackground(QColor(65, 65, 66))
                    sel_result.format.setProperty(QTextCharFormat.FullWidthSelection, True)
                    sel_result.cursor = results_cursor
                    results_selections.append(sel_result)
            
            # Apply basic highlights and return early
            apply_start = self._log_perf("setExtraSelections")
            self.setExtraSelections(selections)
            if hasattr(self.parent, 'results'):
                self.parent.results.setExtraSelections(results_selections)
            self._log_perf("setExtraSelections", apply_start)
            
            self._log_perf("highlightCurrentLine", start_time)
            return
        
        # We have LN references, so do full highlighting
        # Clear previous cross-sheet highlights efficiently
        self.clear_highlighted_sheets_only()
        
        # Build/update cross-sheet cache if needed
        if not self._cross_sheet_cache:
            self.build_cross_sheet_cache()
        
        # Add current line highlight
        sel = QTextEdit.ExtraSelection()
        sel.format.setBackground(QColor(65, 65, 66))
        sel.format.setProperty(QTextCharFormat.FullWidthSelection, True)
        sel.cursor = self.textCursor()
        if not sel.cursor.hasSelection():
            sel.cursor.clearSelection()
        else:
            sel.cursor = QTextCursor(sel.cursor.block())
        selections.append(sel)
        
        # Add operator highlight if it exists
        if self.current_highlight:
            selections.append(self.current_highlight)
        
        # Add result line highlight
        if hasattr(self.parent, 'results'):
            block_number = self.textCursor().blockNumber()
            results_block = self.parent.results.document().findBlockByNumber(block_number)
            if results_block.isValid():
                results_cursor = QTextCursor(results_block)
                sel_result = QTextEdit.ExtraSelection()
                sel_result.format.setBackground(QColor(65, 65, 66))
                sel_result.format.setProperty(QTextCharFormat.FullWidthSelection, True)
                sel_result.cursor = results_cursor
                results_selections.append(sel_result)
        
        calculator = self.get_calculator()
        
        # Collect cross-sheet highlights by sheet (batch operations)
        cross_sheet_highlights = {}
        cross_sheet_results_highlights = {}
        
        # Process each LN reference efficiently
        for match in ln_matches:
            sheet_name = match.group(1)  # Will be None for regular LN refs
            ln_id = int(match.group(2))
            ln_color = self.highlighter.get_ln_color(ln_id)
            bg_color = self.highlighter.get_darker_color(ln_color)
            
            if sheet_name and calculator:  # Cross-sheet reference
                sheet_name_lower = sheet_name.lower()
                
                # Use cached lookup for cross-sheet references
                if sheet_name_lower in self._cross_sheet_cache:
                    sheet_cache = self._cross_sheet_cache[sheet_name_lower]
                    if ln_id in sheet_cache:
                        line_number = sheet_cache[ln_id]
                        
                        # Find the actual sheet widget
                        for i in range(calculator.tabs.count()):
                            if calculator.tabs.tabText(i).lower() == sheet_name_lower:
                                other_sheet = calculator.tabs.widget(i)
                                
                                # Track that this sheet will have highlights
                                self._highlighted_sheets.add(other_sheet)
                                
                                # Create editor highlight
                                doc = other_sheet.editor.document()
                                blk = doc.findBlockByNumber(line_number)
                                if blk.isValid():
                                    highlight_cursor = QTextCursor(blk)
                                    sel_ref = QTextEdit.ExtraSelection()
                                    sel_ref.format.setBackground(bg_color)
                                    sel_ref.format.setProperty(QTextCharFormat.FullWidthSelection, True)
                                    sel_ref.cursor = highlight_cursor
                                    
                                    if other_sheet not in cross_sheet_highlights:
                                        cross_sheet_highlights[other_sheet] = []
                                    cross_sheet_highlights[other_sheet].append(sel_ref)
                                    
                                    # Create results highlight
                                    if hasattr(other_sheet, 'results'):
                                        results_block = other_sheet.results.document().findBlockByNumber(line_number)
                                        if results_block.isValid():
                                            results_cursor = QTextCursor(results_block)
                                            sel_result = QTextEdit.ExtraSelection()
                                            sel_result.format.setBackground(bg_color)
                                            sel_result.format.setProperty(QTextCharFormat.FullWidthSelection, True)
                                            sel_result.cursor = results_cursor
                                            
                                            if other_sheet not in cross_sheet_results_highlights:
                                                cross_sheet_results_highlights[other_sheet] = []
                                            cross_sheet_results_highlights[other_sheet].append(sel_result)
                                break
                            
            else:  # Regular LN reference - use faster lookup
                doc = self.document()
                for i in range(doc.blockCount()):
                    blk = doc.findBlockByNumber(i)
                    user_data = blk.userData()
                    if isinstance(user_data, LineData) and user_data.id == ln_id:
                        # Highlight in editor
                        highlight_cursor = QTextCursor(blk)
                        sel_ref = QTextEdit.ExtraSelection()
                        sel_ref.format.setBackground(bg_color)
                        sel_ref.format.setProperty(QTextCharFormat.FullWidthSelection, True)
                        sel_ref.cursor = highlight_cursor
                        selections.append(sel_ref)
                        
                        # Highlight in results
                        if hasattr(self.parent, 'results'):
                            results_block = self.parent.results.document().findBlockByNumber(i)
                            if results_block.isValid():
                                results_cursor = QTextCursor(results_block)
                                sel_result = QTextEdit.ExtraSelection()
                                sel_result.format.setBackground(bg_color)
                                sel_result.format.setProperty(QTextCharFormat.FullWidthSelection, True)
                                sel_result.cursor = results_cursor
                                results_selections.append(sel_result)
                        break
        
        # Apply all highlights in batch operations
        self.setExtraSelections(selections)
        if hasattr(self.parent, 'results'):
            self.parent.results.setExtraSelections(results_selections)
        
        # Apply cross-sheet highlights in batch
        for other_sheet, highlights in cross_sheet_highlights.items():
            other_sheet.editor.setExtraSelections(highlights)
            if other_sheet in cross_sheet_results_highlights and hasattr(other_sheet, 'results'):
                other_sheet.results.setExtraSelections(cross_sheet_results_highlights[other_sheet])

        self._log_perf("highlightCurrentLine", start_time)

    def _do_highlight_current_line(self):
        """Actual highlighting implementation - called by debounced timer"""
        start_time = self._log_perf("_do_highlight_current_line")
        self._last_highlighted_line = self._last_line
        self.highlightCurrentLine()
        self._log_perf("_do_highlight_current_line", start_time)

    def _do_basic_highlight_only(self):
        """Highlight the current line without processing LN references"""
        selections = []
        results_selections = []
        
        # Basic current line highlight
        sel = QTextEdit.ExtraSelection()
        sel.format.setBackground(QColor(65, 65, 66))
        sel.format.setProperty(QTextCharFormat.FullWidthSelection, True)
        sel.cursor = self.textCursor()
        if not sel.cursor.hasSelection():
            sel.cursor.clearSelection()
        else:
            sel.cursor = QTextCursor(sel.cursor.block())
        selections.append(sel)
        
        # Add operator highlight if it exists
        if self.current_highlight:
            selections.append(self.current_highlight)
        
        # Add result line highlight
        if hasattr(self.parent, 'results'):
            block_number = self.textCursor().blockNumber()
            results_block = self.parent.results.document().findBlockByNumber(block_number)
            if results_block.isValid():
                results_cursor = QTextCursor(results_block)
                sel_result = QTextEdit.ExtraSelection()
                sel_result.format.setBackground(QColor(65, 65, 66))
                sel_result.format.setProperty(QTextCharFormat.FullWidthSelection, True)
                sel_result.cursor = results_cursor
                results_selections.append(sel_result)
        
        # Apply basic highlights only - no LN processing during rapid navigation
        self.setExtraSelections(selections)
        if hasattr(self.parent, 'results'):
            self.parent.results.setExtraSelections(results_selections)

class FormulaEditor(QPlainTextEdit, EditorAutoCompletionMixin, EditorPerformanceMonitoringMixin, EditorCrossSheetMixin, EditorTextSelectionMixin, EditorLineManagementMixin):
    def __init__(self, parent):
        super().__init__(parent)
        self.parent = parent
        self.next_line_id = 1
        self.default_font_size = 14
        self.current_font_size = self.parent.settings.value('font_size', self.default_font_size, type=int)
        
        # Add highlight selection tracking
        self.current_highlight = None  # Store current highlight selection
        
        # Add performance optimizations for highlighting
        self._last_highlighted_line = -1  # Track which line was last highlighted
        self._highlighted_sheets = set()  # Track which sheets have highlights to clear
        self._cross_sheet_cache = {}  # Cache for cross-sheet lookups: sheet_name -> {line_id: line_number}
        self._highlight_timer = QTimer(self)  # Debounce timer for highlighting
        self._highlight_timer.setInterval(100)  # 100ms debounce for more aggressive debouncing
        self._highlight_timer.setSingleShot(True)
        self._highlight_timer.timeout.connect(self._do_highlight_current_line)
        
        # Add rapid navigation detection to avoid highlighting during fast movement
        self._rapid_nav_timer = QTimer(self)
        self._rapid_nav_timer.setInterval(150)  # 150ms to detect end of rapid navigation
        self._rapid_nav_timer.setSingleShot(True)
        self._rapid_nav_timer.timeout.connect(self._end_rapid_navigation)
        self._is_rapid_navigation = False
        self._nav_move_count = 0
        self._last_nav_time = 0
        
        # Add pre-compiled regex patterns for Stage 2 optimization after the existing cache declarations
        # Cache LN reference parsing to avoid regex on every move
        self._line_ln_cache = {}  # line_number -> list of ln_matches
        
        # Stage 2 Optimization: Pre-compiled regex patterns for LN reference processing
        self._ln_normalization_pattern = re.compile(r'\bs\.(.*?)\.ln', re.IGNORECASE)
        self._ln_case_pattern = re.compile(r'\bln(\d+)\b', re.IGNORECASE)
        self._ln_combined_pattern = re.compile(r'\b(S\.)(.*?)\.LN(\d+)\b|\bLN(\d+)\b')
        self._ln_reference_cache = {}  # expr_hash -> processed_expr for caching LN reference processing
        
        # Add debugging tools for performance analysis
        self._debug_enabled = True  # Set to False to disable debugging
        self._perf_log = []  # Store performance measurements
        self._last_perf_time = 0
        self._call_stack = []  # Track what methods are being called
        
        # Add truncate function to the editor instance
        self.truncate = truncate
        
        # Store lines that need separators
        self.separator_lines = set()
        
        # Install key event filter to handle Ctrl key combinations properly
        self.key_filter = KeyEventFilter(self)
        self.installEventFilter(self.key_filter)
        
        # Setup autocompletion first
        self.completion_prefix = ""
        self.completion_list = AutoCompleteList(self)
        self.completion_list.itemClicked.connect(self.complete_text)
        self.completion_list.hide()
        self.setup_autocompletion()

        # Then setup the editor
        self.setFont(QFont("Courier New", self.current_font_size, QFont.Bold))
        self.setStyleSheet("""
            QPlainTextEdit {
                background-color: #2c2c2e; 
                color: white;
                border: none;
                padding: 0px;
                margin: 0px;
                line-height: 1.2em;
            }
            QScrollBar:vertical {
                background: #2c2c2e;
                width: 8px;
                border: none;
            }
            QScrollBar::handle:vertical {
                background: #555555;
                min-height: 20px;
                border-radius: 2px;
            }
            QScrollBar::handle:vertical:hover {
                background: #666666;
            }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
                height: 0px;
            }
            QScrollBar:horizontal {
                background: #2c2c2e;
                height: 8px;
                border: none;
            }
            QScrollBar::handle:horizontal {
                background: #555555;
                min-width: 20px;
                border-radius: 2px;
            }
            QScrollBar::handle:horizontal:hover {
                background: #666666;
            }
            QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
                width: 0px;
            }
        """)
        self.highlighter = FormulaHighlighter(self.document())
        self.lnr = LineNumberArea(self)
        self.blockCountChanged.connect(self.updateLineNumberAreaWidth)
        self.updateRequest.connect(self.updateLineNumberArea)
        self.cursorPositionChanged.connect(self.on_cursor_position_changed)
        self.updateLineNumberAreaWidth(0)
        self.highlightCurrentLine()
        self.setMouseTracking(True)
        self.viewport().installEventFilter(self)
        self.ln_value_map = {}
        self.operator_results_map = {}  # Store operator results for tooltips
        self.setLineWrapMode(QPlainTextEdit.NoWrap)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOn)
        
        # Set document margins to match QTextEdit exactly
        doc = self.document()
        doc.setDocumentMargin(0)
        
        # Set text margins to zero
        self.setViewportMargins(0, 0, 0, 0)
        self.setContentsMargins(0, 0, 0, 0)

    def get_calculator(self):
        """Get the Calculator instance that owns all tabs"""
        # Start with the immediate parent (Worksheet)
        parent = self.parent
        # Keep going up until we find the Calculator instance or run out of parents
        while parent:
            if isinstance(parent, Calculator):
                return parent
            # Get the next parent up the chain
            parent = parent.parent()
        return None

    def update_separator_lines(self):
        """Update the set of lines that need separators above them - DISABLED since we now have blue function highlighting"""
        # self.separator_lines.clear()
        # lines = self.toPlainText().split('\n')
        
        # # List of functions that operate on lines above
        # line_functions = {'sum', 'mean', 'median', 'mode', 'min', 'max', 'range', 
        #                  'count', 'product', 'variance', 'stdev', 'std', 'geomean', 
        #                  'harmmean', 'sumsq', 'perc5', 'perc95'}
        
        # for i, line in enumerate(lines):
        #     # Check for function calls that operate on lines above
        #     match = re.match(r'(\w+)\s*\((.*?)\)', line.strip())
        #     if match:
        #         func_name = match.group(1).lower()
        #         args = match.group(2).strip()
                
        #         if func_name in line_functions:
        #             # If it's a line function, add a separator above this line
        #             self.separator_lines.add(i)
        
        # # Force a viewport update to show the new separators
        # self.viewport().update()
        pass  # No longer needed with function highlighting

    def truncate_func(self, value, decimals=2):
        """Deprecated: Use global truncate function instead"""
        return truncate(value, decimals)

    def lineNumberAreaWidth(self):
        digits = len(str(max(1, self.blockCount())))
        return 3 + self.fontMetrics().horizontalAdvance('9') * digits

    def updateLineNumberAreaWidth(self, _):
        self.setViewportMargins(self.lineNumberAreaWidth() + 6, 0, 0, 0)

    def calculate_subexpression(self, expr):
        """Calculate the result of a subexpression"""
        try:
            # Handle numbers with leading zeros
            expr = re.sub(r'\b0+(\d+)\b', r'\1', expr)
            
            # Process LN references if present
            if re.search(r"\bLN(\d+)\b", expr):
                expr = self.process_ln_refs(expr)
            
            # Handle the expression evaluation using the global truncate function
            result = eval(expr, {"truncate": truncate, "TR": truncate, **GLOBALS}, {})
            
            # Format the result nicely
            if isinstance(result, float):
                # Round to 6 decimal places to avoid floating point noise
                result = round(result, 6)
                # Convert to int if it's a whole number
                if result.is_integer():
                    result = int(result)
            
            return result
            
        except Exception as e:
            return None

    def get_expression_at_operator(self, text, pos):
        """Get the expression inside parentheses at the given operator position"""
        # Find all matching parentheses pairs
        stack = []
        pairs = []
        for i, char in enumerate(text):
            if char == '(':
                stack.append(i)
            elif char == ')' and stack:
                start = stack.pop()
                pairs.append((start, i))
        
        # Sort pairs by size (inner to outer)
        pairs.sort(key=lambda p: p[1] - p[0])
        
        # Find the innermost pair containing our position
        for start, end in pairs:
            if start < pos < end:
                # Get the expression and handle leading zeros
                expr = text[start+1:end]
                # Replace numbers with leading zeros
                expr = re.sub(r'\b0+(\d+)\b', r'\1', expr)
                return expr, (start + 1, end)  # Return both expression and its position
        return None, None

    def on_cursor_position_changed(self):
        start_time = self._log_perf("on_cursor_position_changed")
        
        # Store current cursor and scroll position
        cursor = self.textCursor()
        scrollbar = self.verticalScrollBar()
        current_scroll = scrollbar.value()
        
        # Always update the current line tracking
        current_line = cursor.blockNumber()
        self._last_line = current_line
        self._current_block = cursor.block()
        
        # Detect rapid navigation
        current_time = time.time() * 1000  # Convert to milliseconds
        
        if self._last_nav_time > 0:
            time_diff = current_time - self._last_nav_time
            if time_diff < 100:  # Less than 100ms between moves = rapid navigation
                self._nav_move_count += 1
                if self._nav_move_count >= 2:  # 2+ rapid moves = rapid navigation mode
                    self._is_rapid_navigation = True
                    if self._debug_enabled:
                        print(f"RAPID NAV: Started (move #{self._nav_move_count}, {time_diff:.1f}ms gap)")
            else:
                self._nav_move_count = 0
        
        self._last_nav_time = current_time
        
        # Start rapid navigation timer to detect when user stops
        if self._is_rapid_navigation:
            self._rapid_nav_timer.start()
        
        # Only highlight if we've moved to a different line AND not in rapid navigation
        if (self._last_highlighted_line != current_line and 
            not self._is_rapid_navigation):
            # Use debounced highlighting for normal navigation
            self._highlight_timer.start()
        elif self._last_highlighted_line != current_line and self._is_rapid_navigation:
            # During rapid navigation, just do basic current line highlight without LN processing
            basic_start = self._log_perf("_do_basic_highlight_only")
            self._do_basic_highlight_only()
            self._log_perf("_do_basic_highlight_only", basic_start)
        
        # Trigger scroll synchronization to keep results in sync
        if hasattr(self.parent, '_sync_editor_to_results'):
            sync_start = self._log_perf("_sync_editor_to_results")
            self.parent._sync_editor_to_results(current_scroll)
            self._log_perf("_sync_editor_to_results", sync_start)
            
            # Check for scroll sync issues
            self._check_scroll_sync_issue()
        
        # Hide completion list if there's a selection
        if cursor.hasSelection():
            self.completion_list.hide()
            
        # Only restore cursor position if we don't have a user selection
        # to avoid interfering with manual text selection
        if not cursor.hasSelection():
            self.setTextCursor(cursor)
            
        # Update line number areas to reflect current line highlighting
        self.lnr.update()
        if hasattr(self.parent, 'results_lnr'):
            self.parent.results_lnr.update()
            
        self._log_perf("on_cursor_position_changed", start_time)

    def find_operator_results(self, text, block_position):
        """Find all operators and calculate their subexpression results"""
        self.operator_results_map.clear()
        
        # First, find all parentheses pairs and build a map of their relationships
        stack = []
        paren_pairs = {}  # Maps opening paren position to closing paren position
        paren_contents = {}  # Maps opening paren position to its contents
        
        # Find all matching parentheses first
        for i, char in enumerate(text):
            if char == '(':
                stack.append(i)
            elif char == ')' and stack:
                start = stack.pop()
                paren_pairs[start] = i
                paren_contents[start] = text[start+1:i]
        
        # Now find operators and their associated expressions
        operators = list(re.finditer(r'[-+*/^]', text))
        
        for op_match in operators:
            op_pos = op_match.start()
            op_end = op_match.end()
            
            # Skip whitespace after operator
            next_pos = op_end
            while next_pos < len(text) and text[next_pos].isspace():
                next_pos += 1
                
            # If we find an opening parenthesis after the operator
            if next_pos < len(text) and text[next_pos] == '(':
                # Find the innermost expression for this operator
                if next_pos in paren_pairs:
                    subexpr = paren_contents[next_pos]
                    try:
                        result = self.calculate_subexpression(subexpr)
                        if result is not None:
                            # Store result with the operator's position
                            abs_pos = block_position + op_pos
                            self.operator_results_map[abs_pos] = result
                    except Exception as e:
                        pass

    def eventFilter(self, obj, event):
        if obj is self.completion_list:
            if event.type() == QEvent.KeyPress:
                if event.key() in (Qt.Key_Up, Qt.Key_Down, Qt.Key_Return, Qt.Key_Enter, Qt.Key_Tab, Qt.Key_Escape):
                    self.keyPressEvent(event)
                    return True
        elif obj is self.viewport():
            if event.type() == QEvent.MouseMove:
                cursor = self.cursorForPosition(event.position().toPoint())
                block = cursor.block()
                text = block.text()
                pos = cursor.positionInBlock()

                # Check if we're over an operator
                found_operator = False
                for op_match in re.finditer(r'[-+*/^]', text):
                    op_start = op_match.start()
                    op_end = op_match.end()
                    if op_start <= pos < op_end:
                        found_operator = True
                        # Get the expression inside the current parentheses
                        expr, expr_pos = self.get_expression_at_operator(text, pos)
                        if expr and expr_pos:
                            try:
                                result = self.calculate_subexpression(expr)
                                if result is not None:
                                    # Highlight the expression
                                    self.highlight_expression(block, expr_pos[0], expr_pos[1])
                                    QToolTip.showText(event.globalPosition().toPoint(), f"Result: {result}", self)
                                    return True
                            except Exception as e:
                                pass
                        break

                # Handle LN reference tooltips (including cross-sheet references)
                found_ln_tooltip = False
                
                # First check for cross-sheet references: s.SheetName.ln2 or S.SheetName.LN2
                for match in re.finditer(r'\b(?:s|S)\.(.+?)\.(?:ln|LN)(\d+)\b', text):
                    start, end = match.span()
                    if start <= pos < end:  # Use exclusive end to avoid boundary issues
                        found_operator = True
                        found_ln_tooltip = True
                        sheet_name = match.group(1)
                        ln_id = int(match.group(2))
                        val = self.get_sheet_value(sheet_name, ln_id)
                        if val is not None:
                            display = f"S.{sheet_name}.LN{ln_id} = {val}"
                        else:
                            display = f"S.{sheet_name}.LN{ln_id} not found"
                        QToolTip.showText(event.globalPosition().toPoint(), display, self)
                        break
                
                # If no cross-sheet reference found, check for regular LN references
                if not found_ln_tooltip:
                    for match in re.finditer(r'\bLN(\d+)\b', text):
                        start, end = match.span()
                        if start <= pos < end:  # Use exclusive end to avoid boundary issues
                            found_operator = True
                            found_ln_tooltip = True
                            ln_id = int(match.group(1))
                            val = self.ln_value_map.get(ln_id)
                            if val is not None:
                                display = f"LN{ln_id} = {val}"
                            else:
                                display = f"LN{ln_id} not found"
                            QToolTip.showText(event.globalPosition().toPoint(), display, self)
                            break

                # If we're not over an operator or LN reference, clear highlights
                if not found_operator:
                    self.clear_expression_highlight()
                    QToolTip.hideText()

            elif event.type() == QEvent.Leave:
                # Clear highlight when mouse leaves the viewport
                self.clear_expression_highlight()
                QToolTip.hideText()

        return super().eventFilter(obj, event)

    @Slot(QRect, int)
    def updateLineNumberArea(self, rect, dy):
        if dy:
            self.lnr.scroll(0, dy)
        else:
            self.lnr.update(0, rect.y(), self.lnr.width(), rect.height())
        if rect.contains(self.viewport().rect()):
            self.updateLineNumberAreaWidth(0)

    def resizeEvent(self, event):
        super().resizeEvent(event)
        cr = self.contentsRect()
        self.lnr.setGeometry(cr.x(), cr.y(), self.lineNumberAreaWidth(), cr.height())

    def lineNumberAreaPaintEvent(self, event):
        painter = QPainter(self.lnr)
        painter.fillRect(event.rect(), QColor("#1e1e1e"))
        block = self.firstVisibleBlock()
        top = self.blockBoundingGeometry(block).translated(self.contentOffset()).top()
        bottom = top + self.blockBoundingRect(block).height()
        
        # Get the current line number for highlighting
        current_block_number = self.textCursor().blockNumber()
        
        while block.isValid() and top <= event.rect().bottom():
            if block.isVisible() and bottom >= event.rect().top():
                txt = block.text().strip()
                data = block.userData()
                label = "C" if txt.startswith(":::") else str(data.id if data else block.blockNumber()+1)
                color = "#7ED321" if txt.startswith(":::") else "#888"
                
                # Check if this is the current line - make it bold and white
                is_current_line = block.blockNumber() == current_block_number
                if is_current_line:
                    color = "#FFFFFF"  # White for current line
                    # Set bold font
                    font = painter.font()
                    font.setBold(True)
                    painter.setFont(font)
                else:
                    # Ensure font is not bold for other lines
                    font = painter.font()
                    font.setBold(False)
                    painter.setFont(font)
                
                painter.setPen(QColor(color))
                painter.drawText(0, int(top), self.lnr.width()-2, self.fontMetrics().height(), Qt.AlignRight, label)
            block = block.next()
            top = bottom
            bottom = top + self.blockBoundingRect(block).height()

    def clear_cross_sheet_highlights(self):
        """Clear all cross-sheet highlights from other sheets"""
        calculator = self.get_calculator()
        if not calculator:
            return
            
        # Clear highlights from all other sheets
        for i in range(calculator.tabs.count()):
            other_sheet = calculator.tabs.widget(i)
            if other_sheet != self.parent and hasattr(other_sheet, 'editor'):
                # Clear all extra selections (cross-sheet highlights)
                other_sheet.editor.setExtraSelections([])

    def _end_rapid_navigation(self):
        """Mark end of rapid navigation period and trigger full highlighting"""
        if self._debug_enabled:
            print(f"RAPID NAV: Ended")
        self._is_rapid_navigation = False
        self._nav_move_count = 0
        self._last_nav_time = 0
        
        # Trigger full highlighting now that rapid navigation has ended
        if self._last_highlighted_line != self._last_line:
            self._highlight_timer.start()

    def _handle_unit_conversion(self, expr):
        """Handle unit conversion expressions like '1 mile to km'"""
        pattern = r'(\d+(?:\.\d+)?)\s+(\w+)\s+to\s+(\w+)'
        match = re.match(pattern, expr.lower())
        if match:
            value, from_unit, to_unit = match.groups()
            # Handle unit abbreviations
            from_unit = UNIT_ABBR.get(from_unit.lower(), from_unit)
            to_unit = UNIT_ABBR.get(to_unit.lower(), to_unit)
            try:
                # Create quantity and convert
                q = ureg.Quantity(float(value), from_unit)
                result = q.to(to_unit)
                # Get the full spelling for display
                display_unit = UNIT_DISPLAY.get(to_unit, to_unit)
                # Return both the value and the unit
                return {'value': float(result.magnitude), 'unit': display_unit}
            except:
                return None
        return None

    def _evaluate_lines(self, lines, vals, out, doc):
        """Evaluate each line and populate results"""
        # Evaluate each line
        for idx, line in enumerate(lines):
            self.current_line = line  # Store current line for context
            s = line.strip()
            if not s:  # Empty line
                vals[idx] = None
                blk = doc.findBlockByNumber(idx)
                data = blk.userData()
                if isinstance(data, LineData):
                    self.editor.ln_value_map[data.id] = vals[idx]
                out.append("")
                continue
            
            if s.startswith(":::"):  # Comment line
                vals[idx] = None
                blk = doc.findBlockByNumber(idx)
                data = blk.userData()
                if isinstance(data, LineData):
                    self.editor.ln_value_map[data.id] = vals[idx]
                out.append("")
                continue

            # Try special cases first
            try:
                # Get the current block and its ID
                blk = doc.findBlockByNumber(idx)
                data = blk.userData()
                current_id = data.id if isinstance(data, LineData) else None

                # Pre-process the expression to handle padded numbers
                s = self._preprocess_expression(s)

                # Check for D() function call first
                d_func_match = re.match(r'D\((.*?)\)', s)
                if d_func_match:
                    # Extract the content inside D() and process it directly
                    date_content = d_func_match.group(1)
                    date_result = handle_date_arithmetic(date_content)
                    
                    if date_result is not None:
                        vals[idx] = date_result
                        if current_id:
                            self.editor.ln_value_map[current_id] = vals[idx]
                        out.append(self.format_number_for_display(date_result, idx + 1))
                        continue

                # Check for unit conversion
                unit_result = self._handle_unit_conversion(s)
                if unit_result is not None:
                    vals[idx] = unit_result
                    if current_id:
                        self.editor.ln_value_map[current_id] = vals[idx]
                    out.append(self.format_number_for_display(unit_result, idx + 1))
                    continue

                # Check for currency conversion
                currency_result = handle_currency_conversion(s)
                if currency_result is not None:
                    vals[idx] = currency_result
                    if current_id:
                        self.editor.ln_value_map[current_id] = vals[idx]
                    out.append(self.format_number_for_display(currency_result, idx + 1))
                    continue

                # Check for truncate function call (both truncate and TR)
                trunc_match = re.match(r'(?:truncate|TR)\((.*?),(.*?)\)', s)
                if trunc_match:
                    # First evaluate the expression
                    expr = self.editor.process_ln_refs(trunc_match.group(1).strip())
                    decimals = int(eval(trunc_match.group(2).strip(), {"truncate": truncate, "TR": truncate, **GLOBALS}, {}))
                    
                    # Try unit conversion first
                    unit_result = self._handle_unit_conversion(expr)
                    if unit_result is not None:
                        v = truncate(unit_result, decimals)
                    else:
                        # Try currency conversion
                        currency_result = handle_currency_conversion(expr)
                        if currency_result is not None:
                            v = truncate(currency_result, decimals)
                        else:
                            # If not a unit or currency conversion, evaluate as regular expression
                            val = eval(expr, {"truncate": truncate, "TR": truncate, **GLOBALS}, {})
                            v = truncate(val, decimals)
                        
                    vals[idx] = v
                    if current_id:
                        self.editor.ln_value_map[current_id] = vals[idx]
                    out.append(self.format_number_for_display(v, idx + 1))
                    continue

                # Try special commands
                cmd_result = self._handle_special_commands(s, idx, lines, vals)
                if cmd_result is not None:
                    vals[idx] = cmd_result
                    if current_id:
                        self.editor.ln_value_map[current_id] = vals[idx]
                    out.append(self.format_number_for_display(cmd_result, idx + 1))
                    continue

                # Process LN references if present
                if re.search(r"\b(?:s\.|S\.)?(?:ln|LN)\d+\b", s, re.IGNORECASE):
                    s = self.editor.process_ln_refs(s)
                    # print(f"Line {idx + 1} after processing refs: {s}")  # Debug print - commented for performance

                # Try to evaluate the expression with math functions
                v = eval(s, {"truncate": truncate, "mean": statistics.mean, "TR": truncate, **GLOBALS}, {})
                vals[idx] = v
                if current_id:
                    self.editor.ln_value_map[current_id] = vals[idx]
                    # print(f"Stored value {v} for line ID {current_id}")  # Debug print - commented for performance
                
                # Format the output
                out.append(self.format_number_for_display(v, idx + 1))
            except TimecodeError as e:
                # Handle TimecodeError specifically to show the actual error message
                # print(f"Timecode error on line {idx + 1}: {str(e)}")  # Debug print - commented for performance
                vals[idx] = None
                if current_id:
                    self.editor.ln_value_map[current_id] = None
                out.append(f'TC ERROR: {str(e)}')
            except Exception as e:
                # print(f"Error evaluating line {idx + 1}: {str(e)}")  # Debug print - commented for performance
                vals[idx] = None
                if current_id:
                    self.editor.ln_value_map[current_id] = None
                out.append('ERROR!')

    def keyPressEvent(self, event):
        # Handle special key combinations first
        modifiers = event.modifiers()
        key = event.key()

        # Handle Ctrl+Shift+Delete as emergency clear for mass delete issues
        if key == Qt.Key_Delete and modifiers == (Qt.ControlModifier | Qt.ShiftModifier):
            print("DEBUG: Emergency clear triggered - clearing both editor and results")
            self.setPlainText("")
            # Find the worksheet parent
            worksheet = self.parent()
            while worksheet and not hasattr(worksheet, 'results'):
                worksheet = worksheet.parent()
            if worksheet and hasattr(worksheet, 'results'):
                worksheet.results.setPlainText("")
            event.accept()
            return

        # Handle Delete/Backspace with selection explicitly
        if key in (Qt.Key_Delete, Qt.Key_Backspace):
            cursor = self.textCursor()
            if cursor.hasSelection():
                cursor.removeSelectedText()
                event.accept()
                return

        # Completion list navigation
        if self.completion_list and self.completion_list.isVisible():
            if self.completion_list.handle_key_event(key):
                event.accept()
                return
        
        # Ctrl+C: Copy answer if no selection, otherwise let KeyEventFilter handle it
        if modifiers & Qt.ControlModifier and key == Qt.Key_C:
            cursor = self.textCursor()
            if not cursor.hasSelection():
                # No selection - copy the raw numeric value from current line (without units)
                block_number = cursor.blockNumber()
                line_number = block_number + 1  # raw_values uses 1-based indexing

                # First try to get the raw numeric value
                if hasattr(self.parent, 'raw_values') and line_number in self.parent.raw_values:
                    raw_value = self.parent.raw_values[line_number]
                    clipboard = QApplication.clipboard()
                    clipboard.setText(str(raw_value))
                elif hasattr(self.parent, 'results'):
                    # Fallback to formatted result if no raw value available
                    results_doc = self.parent.results.document()
                    results_block = results_doc.findBlockByNumber(block_number)
                    if results_block.isValid():
                        result_text = results_block.text().strip()
                        if result_text:
                            clipboard = QApplication.clipboard()
                            clipboard.setText(result_text)
                event.accept()
                return
            else:
                # Has selection - let KeyEventFilter handle this
                super().keyPressEvent(event)
                return
        
        # Alt+C: Copy the current expression line
        elif modifiers & Qt.AltModifier and key == Qt.Key_C:
            cursor = self.textCursor()
            block = cursor.block()
            line_text = block.text()
            if line_text.strip():
                clipboard = QApplication.clipboard()
                clipboard.setText(line_text)
            event.accept()
            return
        
        # Ctrl+Up: Navigate and select text inside parentheses
        elif modifiers & Qt.ControlModifier and key == Qt.Key_Up:
            self.expand_selection_with_parens()
            event.accept()
            return
            
        # Ctrl+Down: Select entire line
        elif modifiers & Qt.ControlModifier and key == Qt.Key_Down:
            self.select_entire_line()
            event.accept()
            return
            
        # Tab key: Disable tab insertion, instead trigger autocompletion or do nothing
        elif key == Qt.Key_Tab:
            # If completion popup is open, handle tab there
            if self.completion_list and self.completion_list.isVisible():
                self.complete_text()
            # Otherwise, do nothing (disable tab insertion)
            event.accept()
            return
        
        # Ctrl+Shift+Left/Right: Navigate between worksheet tabs
        elif modifiers & Qt.ControlModifier and modifiers & Qt.ShiftModifier and key in (Qt.Key_Left, Qt.Key_Right):
            # Get the calculator instance and handle tab navigation directly
            calculator = self.get_calculator()
            if calculator:
                current_index = calculator.tabs.currentIndex()
                if key == Qt.Key_Left:
                    # Navigate to previous tab
                    if current_index > 0:
                        calculator.tabs.setCurrentIndex(current_index - 1)
                    else:
                        # Wrap to last tab
                        calculator.tabs.setCurrentIndex(calculator.tabs.count() - 1)
                elif key == Qt.Key_Right:
                    # Navigate to next tab
                    if current_index < calculator.tabs.count() - 1:
                        calculator.tabs.setCurrentIndex(current_index + 1)
                    else:
                        # Wrap to first tab
                        calculator.tabs.setCurrentIndex(0)
            event.accept()
            return
        
        # For regular text input, show completion popup after processing
        if key == Qt.Key_Space or (key >= Qt.Key_A and key <= Qt.Key_Z) or (key >= Qt.Key_0 and key <= Qt.Key_9) or key in (Qt.Key_Period, Qt.Key_Underscore, Qt.Key_Comma):
            # Process the key normally first
            super().keyPressEvent(event)
            
            # Check if the current line is a comment (starts with :::)
            cursor = self.textCursor()
            line_text = cursor.block().text().strip()
            if not line_text.startswith(':::'):
                # Only show completion popup if not in a comment line
                QTimer.singleShot(10, self.show_completion_popup)
            return
            
        # For all other keys, call parent implementation
        super().keyPressEvent(event)

    def clear_ln_reference_cache(self):
        """Clear the LN reference cache - Stage 2 Optimization support method"""
        if hasattr(self, '_ln_reference_cache'):
            self._ln_reference_cache.clear()

    def build_cross_sheet_cache(self):
        """Build cache for fast cross-sheet lookups"""
        calculator = self.get_calculator()
        if not calculator:
            return
            
        self._cross_sheet_cache.clear()
        
        # Build cache for all sheets
        for i in range(calculator.tabs.count()):
            sheet = calculator.tabs.widget(i)
            if sheet != self.parent and hasattr(sheet, 'editor'):
                sheet_name = calculator.tabs.tabText(i).lower()
                sheet_cache = {}
                
                doc = sheet.editor.document()
                for j in range(doc.blockCount()):
                    blk = doc.findBlockByNumber(j)
                    user_data = blk.userData()
                    if isinstance(user_data, LineData):
                        sheet_cache[user_data.id] = j
                        
                self._cross_sheet_cache[sheet_name] = sheet_cache

    def invalidate_all_cross_sheet_caches(self):
        """Invalidate cross-sheet caches in all editor instances"""
        for i in range(self.tabs.count()):
            sheet = self.tabs.widget(i)
            if hasattr(sheet, 'editor'):
                # Clear original cross-sheet cache
                if hasattr(sheet.editor, '_cross_sheet_cache'):
                    sheet.editor._cross_sheet_cache.clear()
                # Stage 2 Fix: Also clear LN reference cache when cross-sheet values change
                if hasattr(sheet.editor, '_ln_reference_cache'):
                    sheet.editor._ln_reference_cache.clear()

class Worksheet(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        layout = QVBoxLayout(self)
        self.splitter = QSplitter(Qt.Horizontal)
        self.settings = QSettings('OpenAI', 'SmartCalc')
        
        # Tab switching optimization - Stage 1: Cross-sheet reference tracking
        self.has_cross_sheet_refs = False  # Track if this sheet contains cross-sheet references
        
        # Stage 1 Performance Optimizations: Smart Change Detection and Caching
        # Line-level change tracking for efficient evaluation
        self._last_lines = []  # Store lines from previous text state
        self._changed_lines = set()  # Track which lines have changed
        self._line_hashes = {}  # Cache line content hashes for quick comparison
        
        # Expression result caching to avoid recomputing unchanged expressions
        self._expression_cache = {}  # line_hash -> (result, formatted_result)
        self._cache_max_size = 1000  # Limit cache size to prevent memory bloat
        
        # Smart evaluation timing based on content type
        self._last_change_time = 0
        self._change_type = 'unknown'  # 'simple_math', 'ln_reference', 'complex', 'whitespace'
        
        # Stage 3.1 Performance Optimizations: Line Dependency Graph Infrastructure
        # Track line-to-line dependencies within this sheet (internal references only)
        self.line_dependencies = {}  # {line_num: set of lines that depend on it}
        self.line_references = {}    # {line_num: set of lines it references}
        self.dependency_graph_cache = {}  # Cache for expensive dependency lookups
        
        # Compiled regex for fast LN reference detection (internal only, not cross-sheet)
        self._internal_ln_pattern = re.compile(r'\bLN(\d+)\b', re.IGNORECASE)
        
        # Cache for dependency chain calculations
        self._dependency_chain_cache = {}  # frozenset(changed_lines) -> frozenset(affected_lines)
        
        # Flag to track if dependency graph needs rebuilding
        self._dependency_graph_dirty = True
        
        # Stage 3.3 Performance Optimizations: Dependency-Aware Caching System
        # Cache for storing line results with dependency fingerprints
        self._line_result_cache = {}  # {line_num: {'result': value, 'dependencies': {line_num: hash}, 'hash': content_hash}}
        # Keep track of lines that rely on each result for efficient invalidation
        self._result_dependencies = {}  # {line_num: set(dependent_line_nums)}
        # Store dependency fingerprints to efficiently check if dependencies have changed
        self._dependency_fingerprints = {}  # {line_num: {dependency_line_num: content_hash}}
        
        # Create results widget with custom setPlainText override for mass delete protection
        class ProtectedResultsWidget(QPlainTextEdit):
            def __init__(self, worksheet):
                super().__init__()
                self.worksheet = worksheet

            def setPlainText(self, text):
                # Debug: Log every attempt to set results
                current_editor_text = self.worksheet.editor.toPlainText().strip()
                lines = self.worksheet.editor.toPlainText().split('\n')
                text_lines = text.split('\n') if text else ['']

                print(f"DEBUG: setPlainText called - Editor lines: {len(lines)}, Editor empty: {len(current_editor_text) == 0}, Result lines: {len(text_lines)}")
                if len(text_lines) > 10:
                    print(f"DEBUG: Large result set - first few lines: {text_lines[:3]}")

                # Apply mass delete protection at the widget level - catches ALL attempts to set results
                # Case 1: Single empty line
                if len(lines) == 1 and len(current_editor_text) == 0:
                    print(f"DEBUG: Widget-level protection - Single empty line detected, clearing results")
                    text = ""
                # Case 2: Empty editor but many results (condensed results from other tabs)
                elif len(current_editor_text) == 0 and len(text_lines) > 5:
                    print(f"DEBUG: Widget-level protection - Empty editor with {len(text_lines)} results detected, clearing condensed results")
                    text = ""
                # Case 3: Aggressive protection - Block large result sets that don't match editor line count
                elif len(text_lines) > len(lines) + 10:  # Much more results than editor lines
                    print(f"DEBUG: Widget-level protection - Blocking mismatched large result set: {len(text_lines)} results for {len(lines)} editor lines")
                    text = '\n'.join([''] * len(lines))  # Set empty results matching editor line count

                super().setPlainText(text)

                # Post-evaluation check: Schedule a check after all evaluations are complete
                QTimer.singleShot(100, self.worksheet.check_and_fix_results)

        self.results = ProtectedResultsWidget(self)
        self.results.setReadOnly(True)
        font_size = self.settings.value('font_size', 14, type=int)
        self.results.setFont(QFont("Courier New", font_size, QFont.Bold))
        
        # Use the exact same stylesheet as the editor for consistent scrollbar appearance
        self.results.setStyleSheet("""
            QPlainTextEdit {
                background-color: #2c2c2e; 
                color: white;
                border: none;
                padding: 0px;
                margin: 0px;
                line-height: 1.2em;
            }
            QScrollBar:vertical {
                background: #2c2c2e;
                width: 8px;
                border: none;
            }
            QScrollBar::handle:vertical {
                background: #555555;
                min-height: 20px;
                border-radius: 2px;
            }
            QScrollBar::handle:vertical:hover {
                background: #666666;
            }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
                height: 0px;
            }
            QScrollBar:horizontal {
                background: #2c2c2e;
                height: 8px;
                border: none;
            }
            QScrollBar::handle:horizontal {
                background: #555555;
                min-width: 20px;
                border-radius: 2px;
            }
            QScrollBar::handle:horizontal:hover {
                background: #666666;
            }
            QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
                width: 0px;
            }
        """)
        
        # Copy all the exact same policies as the editor
        self.results.setLineWrapMode(QPlainTextEdit.NoWrap)
        self.results.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOn)
        self.results.setVerticalScrollBarPolicy(Qt.ScrollBarAsNeeded)  # Match editor's vertical policy
        
        # Set document margins to match editor exactly
        results_doc = self.results.document()
        results_doc.setDocumentMargin(0)
        
        # Set text margins to zero
        self.results.setViewportMargins(0, 0, 0, 0)
        self.results.setContentsMargins(0, 0, 0, 0)
        
        # Create line number area for results - make it a child of the results widget directly
        self.results_lnr = ResultsLineNumberArea(self.results)
        
        # Setup line number area connections for results
        self.results.blockCountChanged.connect(self.updateResultsLineNumberAreaWidth)
        self.results.updateRequest.connect(self.updateResultsLineNumberArea)
        # Delay this call until after results_container is created
        # self.updateResultsLineNumberAreaWidth(0)
        
        # Now we can safely call updateResultsLineNumberAreaWidth
        self.updateResultsLineNumberAreaWidth(0)
        
        # Install resize event handler for results widget
        original_results_resize = self.results.resizeEvent
        def results_resize_event(event):
            if original_results_resize:
                original_results_resize(event)
            self.resizeResultsLineNumberArea()
        self.results.resizeEvent = results_resize_event
        
        # Create a container widget for the results with line numbers FIRST
        self.results_container = QWidget()
        self.results_container.setStyleSheet("background-color: #2c2c2e;")
        
        # Setup the results layout immediately
        self.setupResultsLayout()
        
        # Position the line number area correctly
        self.resizeResultsLineNumberArea()
        
        # Add storage for raw values (for copying unformatted numbers)
        self.raw_values = {}  # line_number -> raw_value
        
        # Add syntax highlighter for error coloring
        self.results_highlighter = ResultsHighlighter(self.results.document())
        
        # Then create editor
        self.editor = FormulaEditor(self)
        
        # Add flag to prevent infinite recursion during scroll synchronization
        self._syncing_scroll = False
        
        # Add navigation vs text change tracking to prevent unnecessary evaluations
        self._last_text_content = ""
        self._is_pure_navigation = False
        
        # Connect scrollbars for synchronization
        self.editor.verticalScrollBar().valueChanged.connect(self._sync_editor_to_results)
        self.results.verticalScrollBar().valueChanged.connect(self._sync_results_to_editor)
        
        # Add widgets to splitter
        self.splitter.addWidget(self.editor)
        self.splitter.addWidget(self.results_container)
        self.splitter.setSizes([600, 200])
        layout.addWidget(self.splitter)
        
        # Stage 1: Smart evaluation timer - starts with intelligent timing
        self.timer = QTimer(self)
        self.timer.setSingleShot(True)
        self.timer.timeout.connect(self.evaluate_and_highlight)
        # Dynamic interval set by start_smart_evaluation_timer()

        # Efficient brute force fix - runs continuously with balanced interval
        self.fix_timer = QTimer(self)
        self.fix_timer.timeout.connect(self.efficient_brute_force_fix)
        self.fix_timer.start(300)  # Check every 300ms - good balance of speed vs efficiency
        
        # Add performance flags to prevent excessive evaluation during navigation
        self._is_navigating = False
        self._navigation_timer = QTimer(self)
        self._navigation_timer.setInterval(100)  # Short timer to detect navigation
        self._navigation_timer.setSingleShot(True)
        self._navigation_timer.timeout.connect(self._end_navigation)
        
        # Connect text changes to evaluation
        self.editor.textChanged.connect(self.on_text_potentially_changed)

        # Connect block count changes to ensure line synchronization
        self.editor.blockCountChanged.connect(self.on_editor_block_count_changed)
        
        # Store initial text content for change detection
        self._last_text_content = self.editor.toPlainText()
        # Stage 1: Initialize line tracking
        self._last_lines = self._last_text_content.split('\n')
        self._update_line_hashes()
        
        # Initial evaluation
        QTimer.singleShot(0, self.evaluate_and_highlight)
        
        # Add resize event handling to results container
        original_resize = self.results_container.resizeEvent
        def results_container_resize_event(event):
            if original_resize:
                original_resize(event)
            self.resizeResultsContainer()
        self.results_container.resizeEvent = results_container_resize_event

    def _sync_editor_to_results(self, value):
        """Sync results scrollbar when editor scrollbar changes"""
        if not self._syncing_scroll:
            start_time = None
            if hasattr(self.editor, '_log_perf'):
                start_time = self.editor._log_perf("_sync_editor_to_results")
                
            self._syncing_scroll = True
            try:
                # Get the editor's scrollbar
                editor_scrollbar = self.editor.verticalScrollBar()
                results_scrollbar = self.results.verticalScrollBar()
                
                # Calculate the scroll ratio to handle different content heights
                editor_max = max(1, editor_scrollbar.maximum())
                results_max = max(1, results_scrollbar.maximum())
                
                if editor_max > 0 and results_max > 0:
                    # Calculate proportional position
                    ratio = value / editor_max if editor_max > 0 else 0
                    target_value = int(ratio * results_max)
                    results_scrollbar.setValue(target_value)
                else:
                    # Direct value if no scaling needed
                    results_scrollbar.setValue(value)
            finally:
                self._syncing_scroll = False
                if start_time and hasattr(self.editor, '_log_perf'):
                    self.editor._log_perf("_sync_editor_to_results", start_time)

    def _sync_results_to_editor(self, value):
        """Sync editor scrollbar when results scrollbar changes"""
        if not self._syncing_scroll:
            start_time = None
            if hasattr(self.editor, '_log_perf'):
                start_time = self.editor._log_perf("_sync_results_to_editor")
                
            self._syncing_scroll = True
            try:
                # Get the scrollbars
                editor_scrollbar = self.editor.verticalScrollBar()
                results_scrollbar = self.results.verticalScrollBar()
                
                # Calculate the scroll ratio to handle different content heights
                editor_max = max(1, editor_scrollbar.maximum())
                results_max = max(1, results_scrollbar.maximum())
                
                if results_max > 0 and editor_max > 0:
                    # Calculate proportional position
                    ratio = value / results_max if results_max > 0 else 0
                    target_value = int(ratio * editor_max)
                    editor_scrollbar.setValue(target_value)
                else:
                    # Direct value if no scaling needed
                    editor_scrollbar.setValue(value)
            finally:
                self._syncing_scroll = False
                if start_time and hasattr(self.editor, '_log_perf'):
                    self.editor._log_perf("_sync_results_to_editor", start_time)

    def evaluate_and_highlight(self):
        """Evaluate formulas and ensure highlighting is updated"""
        if hasattr(self.editor, '_debug_enabled') and self.editor._debug_enabled:
            start_time = self.editor._log_perf("evaluate_and_highlight")
            self.evaluate()
            self.editor.highlightCurrentLine()
            self.editor._log_perf("evaluate_and_highlight", start_time)
        else:
            self.evaluate()
            self.editor.highlightCurrentLine()
        
        # Don't clear change flags here - let tab switching logic handle it
        # This was causing cross-sheet updates to fail because flags were cleared
        # immediately after text changes before tab switching could detect them

    def on_editor_block_count_changed(self, new_block_count):
        """Called when the number of lines in the editor changes - handles line synchronization"""
        # Synchronize the results panel to match the editor's line count
        results_doc = self.results.document()
        current_results_count = results_doc.blockCount()

        if current_results_count != new_block_count:
            cursor = QTextCursor(results_doc)
            cursor.beginEditBlock()

            if current_results_count < new_block_count:
                # Need to add lines to results
                cursor.movePosition(QTextCursor.End)
                lines_to_add = new_block_count - current_results_count
                for i in range(lines_to_add):
                    if current_results_count > 0 or i > 0:
                        cursor.insertText('\n')
                    # Insert empty content for the new line

            elif current_results_count > new_block_count:
                # Need to remove lines from results
                if new_block_count == 0:
                    # Clear everything
                    cursor.select(QTextCursor.Document)
                    cursor.removeSelectedText()
                else:
                    # Remove lines from the end, one by one
                    lines_to_remove = current_results_count - new_block_count
                    for i in range(lines_to_remove):
                        # Move to the last block
                        cursor.movePosition(QTextCursor.End)
                        cursor.movePosition(QTextCursor.StartOfBlock)
                        # Select the entire last block including the newline before it (if it exists)
                        if cursor.blockNumber() > 0:
                            # If not the first block, include the newline before it
                            cursor.movePosition(QTextCursor.PreviousCharacter, QTextCursor.KeepAnchor)
                        cursor.movePosition(QTextCursor.EndOfBlock, QTextCursor.KeepAnchor)
                        cursor.removeSelectedText()

                # Clear cached values for deleted lines
                for line_idx in range(new_block_count, current_results_count):
                    if line_idx in self._line_result_cache:
                        self._line_result_cache.pop(line_idx, None)
                    if line_idx in self._dependency_fingerprints:
                        self._dependency_fingerprints.pop(line_idx, None)
                    if line_idx+1 in self.raw_values:  # raw_values uses 1-based indexing
                        self.raw_values.pop(line_idx+1, None)

            cursor.endEditBlock()

            # After modifying the results document, ensure scroll positions stay synchronized
            # This is especially important when adding lines at the bottom while scrolled down
            # Use a timer to delay the sync slightly to allow the editor's auto-scroll to complete
            QTimer.singleShot(10, self._sync_scroll_after_line_change)

    def _sync_scroll_after_line_change(self):
        """Helper method to sync scroll positions after line changes"""
        if hasattr(self, '_sync_editor_to_results'):
            editor_scroll_value = self.editor.verticalScrollBar().value()
            self._sync_editor_to_results(editor_scroll_value)

    def on_text_potentially_changed(self):
        """Called when text might have changed - Stage 1 optimized with smart change detection"""
        # Skip text change processing during mass delete operations for other tabs
        calculator = self.editor.get_calculator()
        if calculator and hasattr(calculator, '_mass_delete_in_progress') and calculator._mass_delete_in_progress:
            # Check if this is the tab that had the mass delete
            mass_delete_tab_index = getattr(calculator, '_mass_delete_tab_index', -1)
            this_tab_index = -1
            for i in range(calculator.tabs.count()):
                if calculator.tabs.widget(i) == self:
                    this_tab_index = i
                    break

            # Only block text change processing for tabs OTHER than the one that had the mass delete
            if this_tab_index != mass_delete_tab_index and this_tab_index != -1:
                print(f"DEBUG: on_text_potentially_changed SKIPPED for tab {this_tab_index} (mass delete was on tab {mass_delete_tab_index}) due to mass delete flag")
                return

        # Get current text content
        current_text = self.editor.toPlainText()

        # Check if text actually changed
        if hasattr(self, '_last_text_content') and current_text == self._last_text_content:
            return  # No actual change

        # Stage 1: Detect which lines changed for smarter evaluation
        old_text = getattr(self, '_last_text_content', '')
        changed_lines = self.detect_changed_lines(old_text, current_text)
        
        # Check for any lines that are now empty and clear their results immediately
        self.clear_results_for_empty_lines(changed_lines)
        
        # Stage 3.1: Update line dependencies for changed lines
        if changed_lines and not self._dependency_graph_dirty:
            old_lines = old_text.split('\n') if old_text else []
            new_lines = current_text.split('\n')
            
            for line_idx in changed_lines:  # changed_lines contains 0-based indices
                if line_idx < len(new_lines):  # Ensure line exists (0-based check)
                    old_content = old_lines[line_idx] if line_idx < len(old_lines) else ''
                    new_content = new_lines[line_idx] if line_idx < len(new_lines) else ''
                    # update_line_dependencies expects 1-based line numbers, so convert
                    self.update_line_dependencies(line_idx + 1, old_content, new_content)
        
        # Stage 1: Check if we should skip evaluation entirely
        if self.should_skip_evaluation(changed_lines):
            self._last_text_content = current_text
            return
        
        # Store new content
        self._last_text_content = current_text
        
        # Get current sheet index
        calculator = self.editor.get_calculator()
        if calculator and hasattr(calculator, '_sheet_changed_flags'):
            current_index = calculator.tabs.currentIndex()
            
            # Set change flag
            calculator._sheet_changed_flags[current_index] = True
            # Debug output can be enabled by setting DEBUG_TAB_SWITCHING = True in Calculator.on_tab_changed()
            # print("TEXT ACTUALLY CHANGED - starting evaluation timer")  # Uncomment for debugging
            
            # Capture undo state after a brief delay to avoid capturing every keystroke
            if hasattr(calculator, 'undo_manager'):
                # Cancel any existing undo capture timer
                if hasattr(self, '_undo_capture_timer'):
                    self._undo_capture_timer.stop()
                else:
                    # Create the timer on first use
                    self._undo_capture_timer = QTimer()
                    self._undo_capture_timer.setSingleShot(True)
                    self._undo_capture_timer.timeout.connect(lambda: calculator.undo_manager.capture_state(calculator))
                
                # Start timer - capture undo state after 1 second of no typing
                self._undo_capture_timer.start(1000)
            
            # Check if cross-sheet references changed
            old_has_refs = getattr(self, 'has_cross_sheet_refs', False)
            
            # Detect cross-sheet references
            cross_sheet_pattern = r'\bS\.[^.]+\.LN\d+\b'
            has_cross_refs = bool(re.search(cross_sheet_pattern, current_text, re.IGNORECASE))
            self.has_cross_sheet_refs = has_cross_refs
            
            # print(f"Cross-sheet refs detected: {has_cross_refs} in sheet {current_index}")  # Uncomment for debugging
            
            # Only rebuild dependency graph if cross-sheet reference structure changed
            if old_has_refs != has_cross_refs or (has_cross_refs and not hasattr(calculator, '_last_dependency_content')):
                # print(f"🔄 Cross-sheet structure changed - rebuilding dependency graph")  # Comment out for normal usage
                calculator.build_dependency_graph()
                calculator._last_dependency_content = current_text
            elif has_cross_refs:
                # Check if the actual cross-sheet references changed (not just any text)
                old_refs = set(re.findall(cross_sheet_pattern, getattr(calculator, '_last_dependency_content', ''), re.IGNORECASE))
                new_refs = set(re.findall(cross_sheet_pattern, current_text, re.IGNORECASE))
                if old_refs != new_refs:
                    # print(f"🔄 Cross-sheet references changed - rebuilding dependency graph")  # Comment out for normal usage
                    calculator.build_dependency_graph()
                    calculator._last_dependency_content = current_text
        
        # Stage 1: Start smart evaluation timer with intelligent timing
        self.start_smart_evaluation_timer(changed_lines)

    def format_number_for_display(self, value, line_number):
        """Format a number for display with commas, storing the raw value separately for copying"""
        try:
            if isinstance(value, dict) and 'value' in value and 'unit' in value:
                # Handle unit conversion results
                num = value['value']
                if isinstance(num, float) and abs(num - round(num)) < 1e-10:
                    num = int(round(num))
                self.raw_values[line_number] = num  # Store raw numeric value
                return f'{num:,} {value["unit"]}'
                
            elif isinstance(value, (int, float)):
                # Check if it's close to an integer
                if isinstance(value, float) and abs(value - round(value)) < 1e-10:
                    value = int(round(value))
                
                self.raw_values[line_number] = value  # Store raw value
                
                if isinstance(value, int):
                    # Format with commas
                    return f'{value:,}'
                else:
                    # Format float with commas, showing at least 8 decimal places for normal numbers
                    abs_val = abs(value)
                    if abs_val >= 1e12:
                        # Very large numbers (> trillion) - use scientific notation
                        return f'{value:,.8g}'
                    elif abs_val < 1e-12 and abs_val > 0:
                        # Very small numbers - use scientific notation
                        return f'{value:.8e}'
                    else:
                        # Normal range numbers - show up to 8 significant decimal places
                        # Use 8 decimal places, then strip trailing zeros
                        display = f'{value:,.8f}'.rstrip('0').rstrip('.')
                        # If we stripped all decimals, add back one zero to show it's a float
                        if '.' not in display and ',' in f'{value:,.8f}':
                            display += '.0'
                        return display
                        
            elif isinstance(value, str) and value.isdigit():
                # Handle string numbers (like frame counts)
                num = int(value)
                self.raw_values[line_number] = num  # Store raw value
                display = f'{num:,}'
                if 'TC(' in self.current_line:  # Add 'frames' suffix for timecode results
                    display += ' frames'
                return display
        except:
            pass
            
        # For non-numeric values, don't store raw value
        return str(value)

    def _preprocess_expression(self, expr):
        """Pre-process expression to handle padded numbers and other special cases"""
        # Handle timecode arithmetic first (BEFORE comma removal to preserve function arguments)
        tc_match = re.match(r'TC\((.*?)\)', expr)
        if tc_match:
            tc_args = tc_match.group(1)
            
            # Split on commas that aren't inside arithmetic expressions
            parts = []
            current = ""
            paren_level = 0
            for char in tc_args:
                if char == ',' and paren_level == 0:
                    parts.append(current.strip())
                    current = ""
                else:
                    if char == '(':
                        paren_level += 1
                    elif char == ')':
                        paren_level -= 1
                    current += char
            if current:
                parts.append(current.strip())
            
            # Process each part
            processed_parts = []
            for i, part in enumerate(parts):
                # Skip the first part (fps)
                if i == 0:
                    processed_parts.append(part)
                    continue
                    
                # Handle arithmetic in timecode expressions
                if any(op in part for op in '+-*/'):
                    # First convert any timecodes to frame counts
                    def convert_tc(m):
                        tc = m.group(0).replace('.', ':')  # Normalize separators
                        fps = float(parts[0])  # Get fps from first argument
                        # Use the global timecode_to_frames function
                        return str(globals()['timecode_to_frames'](tc, fps))
                    # Match both . and : as separators
                    part = re.sub(r'\d{1,2}[:.]\d{1,2}[:.]\d{1,2}[:.]\d{1,2}', convert_tc, part)
                    # Then evaluate the arithmetic
                    try:
                        result = eval(part)
                        processed_parts.append(str(result))
                    except:
                        processed_parts.append(part)
                else:
                    # For non-arithmetic parts, check if it's a timecode and quote it
                    if re.match(r'\d{1,2}[:.]\d{1,2}[:.]\d{1,2}[:.]\d{1,2}', part):
                        # Quote the timecode string
                        part = f'"{part}"'
                    # If it's a frame number, leave it as is
                    elif part.isdigit():
                        pass
                    # If it looks like a timecode but might have spaces, clean it up and quote it
                    elif re.search(r'\d{1,2}\s*[:.]\s*\d{1,2}\s*[:.]\s*\d{1,2}\s*[:.]\s*\d{1,2}', part):
                        cleaned = re.sub(r'\s+', '', part).replace('.', ':')
                        part = f'"{cleaned}"'
                    processed_parts.append(part)
            
            # Reconstruct the TC call
            expr = f"TC({','.join(processed_parts)})"
        
        # Handle aspect ratio calculations
        ar_match = re.match(r'AR\((.*?)\)', expr, re.IGNORECASE)
        if ar_match:
            ar_args = ar_match.group(1)
            # Split on comma
            parts = [part.strip() for part in ar_args.split(',')]
            
            if len(parts) == 2:
                # Quote both parts since they contain dimension strings
                quoted_parts = [f'"{part}"' for part in parts]
                expr = f"AR({','.join(quoted_parts)})"
        
        # Handle commas in numbers (thousands separators) - but avoid function calls
        # More careful pattern that doesn't match numbers inside parentheses
        # Pattern to match numbers with commas like 1,234 or 1,234.56
        # Use negative lookbehind to avoid matching inside function calls
        # This pattern avoids matching numbers that come after an opening parenthesis
        comma_number_pattern = r'(?<!\()\b\d{1,3}(?:,\d{3})*(?:\.\d+)?\b(?![^()]*\))'
        expr = re.sub(comma_number_pattern, remove_thousands_commas, expr)
        
        # Replace numbers with leading zeros outside of timecodes and quoted strings
        expr = re.sub(r'\b0+(\d+)\b', repl_num, expr)
        
        return expr

    def _handle_special_commands(self, expr, idx, lines, vals):
        """Handle special commands like sum() and mean(), with timecode support for min/max/mean"""
        # Extract range or list from parentheses
        match = re.match(r'(\w+)\((.*?)\)', expr.strip())
        if not match:
            return None
        
        cmd_type, args = match.groups()
        cmd_type = cmd_type.lower()  # Make case-insensitive
        
        # Check if this is a special command
        if cmd_type not in ('sum', 'mean', 'meanfps', 'median', 'mode', 'min', 'max', 'range', 'count', 
                          'product', 'variance', 'stdev', 'std', 'geomean', 'harmmean', 
                          'sumsq', 'perc5', 'perc95'):
            return None
        
        # Helper function to detect if a value is a timecode
        def is_timecode(value):
            if isinstance(value, str):
                # Check if it matches timecode pattern HH:MM:SS:FF
                return bool(re.match(r'^\d{1,2}[:.]\d{1,2}[:.]\d{1,2}[:.]\d{1,2}$', value))
            return False
        
        # Helper function to convert timecode to frames (using 24fps as default for comparison)
        def timecode_to_frames_for_comparison(tc_str, fps=24.0):
            try:
                return timecode_to_frames(tc_str, fps)
            except:
                return 0
        
        # Helper function to find nearest comment line above
        def find_comment_above(start_idx):
            for i in range(start_idx - 1, -1, -1):
                if i < len(lines):
                    line_text = lines[i].strip()
                    if line_text.startswith(":::"):
                        return i
            return -1  # No comment found, go to beginning
        
        # Helper function to find nearest comment line below
        def find_comment_below(start_idx):
            for i in range(start_idx + 1, len(lines)):
                line_text = lines[i].strip()
                if line_text.startswith(":::"):
                    return i
            return len(lines)  # No comment found, go to end
        
        # Helper function to temporarily evaluate a line if it hasn't been evaluated yet
        def evaluate_line_if_needed(line_idx):
            if line_idx >= len(vals) or line_idx >= len(lines):
                return None
                
            # If already evaluated, return existing value
            if vals[line_idx] is not None:
                return vals[line_idx]
            
            # Skip empty lines and comments
            line_text = lines[line_idx].strip()
            if not line_text or line_text.startswith(":::"):
                return None
            
            # Try to evaluate the line temporarily
            try:
                # Pre-process the expression
                processed_line = self._preprocess_expression(line_text)
                
                # Handle special cases that need evaluation context
                if re.search(r"\b(?:s\.|S\.)?(?:ln|LN)\d+\b", processed_line, re.IGNORECASE):
                    # This line has LN references, we can't evaluate it safely here
                    return None
                
                # Try simple evaluation
                result = eval(processed_line, {"truncate": truncate, "TR": truncate, **GLOBALS}, {})
                return result
            except:
                return None
        
        # Function to get values from a range of lines (supporting both numbers and timecodes)
        def get_values_from_range(start_end, timecode_mode=False):
            values = []
            try:
                # Handle special keywords
                if start_end.lower() == 'above':
                    # All lines above current
                    for i in range(idx):
                        if i < len(vals) and vals[i] is not None:
                            if timecode_mode or is_timecode(vals[i]):
                                values.append(vals[i])
                            elif isinstance(vals[i], (int, float)):
                                values.append(vals[i])
                elif start_end.lower() == 'below':
                    # All lines below current - evaluate if needed
                    for i in range(idx + 1, len(lines)):
                        value = evaluate_line_if_needed(i)
                        if value is not None:
                            if timecode_mode or is_timecode(value):
                                values.append(value)
                            elif isinstance(value, (int, float)):
                                values.append(value)
                elif start_end.lower() == 'cg-above':
                    # From current line to nearest comment above
                    comment_idx = find_comment_above(idx)
                    start_line = comment_idx + 1 if comment_idx >= 0 else 0
                    for i in range(start_line, idx):
                        if i < len(vals) and vals[i] is not None:
                            if timecode_mode or is_timecode(vals[i]):
                                values.append(vals[i])
                            elif isinstance(vals[i], (int, float)):
                                values.append(vals[i])
                elif start_end.lower() == 'cg-below':
                    # From current line to nearest comment below - evaluate if needed
                    comment_idx = find_comment_below(idx)
                    for i in range(idx + 1, comment_idx):
                        value = evaluate_line_if_needed(i)
                        if value is not None:
                            if timecode_mode or is_timecode(value):
                                values.append(value)
                            elif isinstance(value, (int, float)):
                                values.append(value)
                elif '-' in start_end and ',' not in start_end:
                    # Range notation like "1-5"
                    start, end = map(int, start_end.split('-'))
                    for i in range(start-1, end):
                        if i < len(vals) and vals[i] is not None:
                            if timecode_mode or is_timecode(vals[i]):
                                values.append(vals[i])
                            elif isinstance(vals[i], (int, float)):
                                values.append(vals[i])
                        elif i >= len(vals) or vals[i] is None:
                            # Try to evaluate if not yet processed
                            value = evaluate_line_if_needed(i)
                            if value is not None:
                                if timecode_mode or is_timecode(value):
                                    values.append(value)
                                elif isinstance(value, (int, float)):
                                    values.append(value)
                else:
                    # Comma-separated line numbers like "1,3,5"
                    for arg in start_end.split(','):
                        line_num = int(arg.strip()) - 1
                        if line_num < len(vals) and vals[line_num] is not None:
                            if timecode_mode or is_timecode(vals[line_num]):
                                values.append(vals[line_num])
                            elif isinstance(vals[line_num], (int, float)):
                                values.append(vals[line_num])
                        elif line_num >= len(vals) or vals[line_num] is None:
                            # Try to evaluate if not yet processed
                            value = evaluate_line_if_needed(line_num)
                            if value is not None:
                                if timecode_mode or is_timecode(value):
                                    values.append(value)
                                elif isinstance(value, (int, float)):
                                    values.append(value)
            except:
                pass
            return values

        # Special handling for meanfps function with timecode support
        if cmd_type == 'meanfps':
            # meanfps(fps, range) format for timecode averaging
            args_list = [arg.strip() for arg in args.split(',') if arg.strip()]
            
            if len(args_list) < 1:
                return "ERROR: meanfps requires fps parameter: meanfps(fps, range)"
            
            try:
                fps = float(args_list[0])
                range_args = ','.join(args_list[1:]) if len(args_list) > 1 else ''
                
                # Get values for the range
                if not range_args:
                    # Empty range, use all lines above
                    values = []
                    for i in range(idx):
                        if vals[i] is not None:
                            values.append(vals[i])
                else:
                    values = get_values_from_range(range_args, timecode_mode=True)
                
                if not values:
                    return None
                
                # Convert all timecodes to frame counts
                frame_counts = []
                for v in values:
                    if is_timecode(v):
                        try:
                            frame_counts.append(timecode_to_frames(v, fps))
                        except:
                            continue
                    elif isinstance(v, (int, float)):
                        # If it's already a number, assume it's frame count
                        frame_counts.append(int(v))
                
                if not frame_counts:
                    return None
                
                # Calculate mean frame count
                mean_frames = sum(frame_counts) / len(frame_counts)
                
                # Truncate to integer
                mean_frames_int = int(round(mean_frames))
                
                # Convert back to timecode using the provided fps
                try:
                    result_timecode = frames_to_timecode(mean_frames_int, fps)
                    return result_timecode
                except:
                    return None
            except:
                return "ERROR: Invalid fps parameter in meanfps function"
        
        # Simplified mean function (numbers only)
        if cmd_type == 'mean':
            # Get values from range
            if not args.strip():
                # Empty parentheses, use all lines above
                values = []
                for i in range(idx):
                    if vals[i] is not None:
                        if isinstance(vals[i], (int, float)):
                            values.append(float(vals[i]))
                        elif is_timecode(vals[i]):
                            return "ERROR: Timecode detected - use meanfps(fps, range) for timecode averaging"
            else:
                values = get_values_from_range(args)
                if isinstance(values, str):  # Error message
                    return values
                values = [float(v) for v in values if isinstance(v, (int, float))]
            
            if not values:
                return None
            
            return sum(values) / len(values)
        
        # Handle min and max with timecode support
        if cmd_type in ('min', 'max'):
            # Get values from range
            if not args.strip():
                # Empty parentheses, use all lines above
                values = []
                for i in range(idx):
                    if vals[i] is not None:
                        values.append(vals[i])
            else:
                values = get_values_from_range(args)
            
            if not values:
                return None
            
            # Check if any values are timecodes
            has_timecodes = any(is_timecode(v) for v in values)
            
            if has_timecodes:
                # All values should be timecodes for consistent comparison
                timecode_values = [v for v in values if is_timecode(v)]
                if len(timecode_values) != len(values):
                    # Mixed timecode and numeric values
                    return f"ERROR: {cmd_type.upper()} function cannot mix timecode and numeric values"
                
                # Convert to frames for comparison (using 24fps as default)
                timecode_frames = []
                for tc in timecode_values:
                    try:
                        frames = timecode_to_frames_for_comparison(tc, 24.0)
                        timecode_frames.append((frames, tc))
                    except:
                        continue
                
                if not timecode_frames:
                    return None
                
                # Find min or max based on frame count
                if cmd_type == 'min':
                    result = min(timecode_frames, key=lambda x: x[0])
                else:  # max
                    result = max(timecode_frames, key=lambda x: x[0])
                
                return result[1]  # Return the original timecode string
            else:
                # Regular numeric values
                numbers = [float(v) for v in values if isinstance(v, (int, float))]
                if not numbers:
                    return None
                
                if cmd_type == 'min':
                    return min(numbers)
                else:  # max
                    return max(numbers)
        
        # Regular processing for other functions (numbers only)
        def get_numbers_from_range(start_end):
            numbers = []
            try:
                # Handle special keywords
                if start_end.lower() == 'above':
                    # All lines above current
                    for i in range(idx):
                        if i < len(vals) and vals[i] is not None:
                            # Extract numeric value from unit conversion results or regular numbers
                            numeric_val = self.editor.get_numeric_value(vals[i])
                            if isinstance(numeric_val, (int, float)):
                                numbers.append(float(numeric_val))
                            elif is_timecode(vals[i]):
                                return "ERROR: Timecode values not supported for this function"
                elif start_end.lower() == 'below':
                    # All lines below current - evaluate if needed
                    for i in range(idx + 1, len(lines)):
                        value = evaluate_line_if_needed(i)
                        if value is not None:
                            # Extract numeric value from unit conversion results or regular numbers
                            numeric_val = self.editor.get_numeric_value(value)
                            if isinstance(numeric_val, (int, float)):
                                numbers.append(float(numeric_val))
                            elif is_timecode(value):
                                return "ERROR: Timecode values not supported for this function"
                elif start_end.lower() == 'cg-above':
                    # From current line to nearest comment above
                    comment_idx = find_comment_above(idx)
                    start_line = comment_idx + 1 if comment_idx >= 0 else 0
                    for i in range(start_line, idx):
                        if i < len(vals) and vals[i] is not None:
                            # Extract numeric value from unit conversion results or regular numbers
                            numeric_val = self.editor.get_numeric_value(vals[i])
                            if isinstance(numeric_val, (int, float)):
                                numbers.append(float(numeric_val))
                            elif is_timecode(vals[i]):
                                return "ERROR: Timecode values not supported for this function"
                elif start_end.lower() == 'cg-below':
                    # From current line to nearest comment below - evaluate if needed
                    comment_idx = find_comment_below(idx)
                    for i in range(idx + 1, comment_idx):
                        value = evaluate_line_if_needed(i)
                        if value is not None:
                            # Extract numeric value from unit conversion results or regular numbers
                            numeric_val = self.editor.get_numeric_value(value)
                            if isinstance(numeric_val, (int, float)):
                                numbers.append(float(numeric_val))
                            elif is_timecode(value):
                                return "ERROR: Timecode values not supported for this function"
                elif '-' in start_end and ',' not in start_end:
                    # Range notation like "1-5"
                    start, end = map(int, start_end.split('-'))
                    for i in range(start-1, end):
                        if i < len(vals) and vals[i] is not None:
                            # Extract numeric value from unit conversion results or regular numbers
                            numeric_val = self.editor.get_numeric_value(vals[i])
                            if isinstance(numeric_val, (int, float)):
                                numbers.append(float(numeric_val))
                            elif is_timecode(vals[i]):
                                return "ERROR: Timecode values not supported for this function"
                else:
                    # Comma-separated line numbers like "1,3,5"
                    for arg in start_end.split(','):
                        line_num = int(arg.strip()) - 1
                        if line_num < len(vals) and vals[line_num] is not None:
                            # Extract numeric value from unit conversion results or regular numbers
                            numeric_val = self.editor.get_numeric_value(vals[line_num])
                            if isinstance(numeric_val, (int, float)):
                                numbers.append(float(numeric_val))
                            elif is_timecode(vals[line_num]):
                                return "ERROR: Timecode values not supported for this function"
                        elif line_num >= len(vals) or vals[line_num] is None:
                            # Try to evaluate if not yet processed
                            value = evaluate_line_if_needed(line_num)
                            if value is not None:
                                # Extract numeric value from unit conversion results or regular numbers
                                numeric_val = self.editor.get_numeric_value(value)
                                if isinstance(numeric_val, (int, float)):
                                    numbers.append(float(numeric_val))
                                elif is_timecode(value):
                                    return "ERROR: Timecode values not supported for this function"
            except:
                pass
            return numbers

        # If no arguments provided, use all lines above
        if not args.strip():
            numbers = []
            for i in range(idx):
                if vals[i] is not None:
                    # Extract numeric value from unit conversion results or regular numbers
                    numeric_val = self.editor.get_numeric_value(vals[i])
                    if isinstance(numeric_val, (int, float)):
                        numbers.append(float(numeric_val))
                    elif is_timecode(vals[i]) and cmd_type not in ('min', 'max', 'mean', 'meanfps'):
                        return f"ERROR: Timecode values not supported for {cmd_type.upper()} function"
        else:
            numbers = get_numbers_from_range(args)
            if isinstance(numbers, str):  # Error message
                return numbers
        
        if not numbers:
            return 0 if cmd_type == 'sum' else None

        # Apply the appropriate operation
        try:
            if cmd_type == 'sum':
                return sum(numbers)
            elif cmd_type == 'median':
                return statistics.median(numbers)
            elif cmd_type == 'mode':
                try:
                    return statistics.mode(numbers)
                except statistics.StatisticsError:
                    return None  # No unique mode
            elif cmd_type == 'range':
                return max(numbers) - min(numbers)
            elif cmd_type == 'count':
                return len(numbers)
            elif cmd_type == 'product':
                result = 1
                for n in numbers:
                    result *= n
                return result
            elif cmd_type == 'variance':
                return statistics.variance(numbers)
            elif cmd_type in ('stdev', 'std'):
                return statistics.stdev(numbers)
            elif cmd_type == 'geomean':
                return statistics.geometric_mean(numbers)
            elif cmd_type == 'harmmean':
                return statistics.harmonic_mean(numbers)
            elif cmd_type == 'sumsq':
                return sum(x*x for x in numbers)
            elif cmd_type == 'perc5':
                return statistics.quantiles(numbers, n=20)[0]  # 5th percentile
            elif cmd_type == 'perc95':
                return statistics.quantiles(numbers, n=20)[18]  # 95th percentile
        except:
            return None
        
        return None

    def evaluate(self):
        """Evaluate formulas and update results"""
        # Skip evaluation during mass delete operations for other tabs
        calculator = None
        parent = self.parent()
        while parent and not hasattr(parent, 'tabs'):
            parent = parent.parent()
        if parent and hasattr(parent, 'tabs'):
            calculator = parent
            if hasattr(calculator, '_mass_delete_in_progress') and calculator._mass_delete_in_progress:
                # Check if this is the tab that had the mass delete
                mass_delete_tab_index = getattr(calculator, '_mass_delete_tab_index', -1)
                this_tab_index = -1
                for i in range(calculator.tabs.count()):
                    if calculator.tabs.widget(i) == self:
                        this_tab_index = i
                        break

                # Only block evaluations for tabs OTHER than the one that had the mass delete
                if this_tab_index != mass_delete_tab_index and this_tab_index != -1:
                    print(f"DEBUG: evaluate() SKIPPED for tab {this_tab_index} (mass delete was on tab {mass_delete_tab_index}) due to mass delete flag")
                    return

        # Stage 3.2: Try selective evaluation first if beneficial
        if self._should_use_selective_evaluation():
            if self._try_selective_evaluation():
                return  # Selective evaluation succeeded
        
        # Fall back to full evaluation
        # Initialize evaluation state and data structures
        evaluation_context = self._initialize_evaluation()
        
        # Evaluate each line using the extracted method
        out = self._evaluate_lines_loop(evaluation_context['lines'], evaluation_context['vals'], evaluation_context['doc'])

        # Finalize evaluation and update UI
        self._finalize_evaluation(out, evaluation_context)

    def _should_use_selective_evaluation(self):
        """Determine if selective evaluation is beneficial"""
        # Get current text to analyze changes
        current_text = self.editor.toPlainText()
        
        # Check if we have previous state to compare
        if not hasattr(self, '_last_evaluation_text'):
            self._last_evaluation_text = current_text
            return False
        
        # Detect changed lines
        changed_lines = self.detect_changed_lines(self._last_evaluation_text, current_text)
        if not changed_lines:
            return False  # No changes
        
        total_lines = len(current_text.split('\n'))
        
        # Skip selective evaluation for small sheets (overhead not worth it)
        if total_lines < 10:
            return False
        
        # Skip if too many lines changed (selective evaluation won't help much)
        change_ratio = len(changed_lines) / max(1, total_lines)
        if change_ratio > 0.3:  # More than 30% changed
            return False
        
        # Check dependency impact - if dependents affect too much, fall back to full
        try:
            dependency_chain = self.get_dependency_chain(changed_lines)
            dependency_ratio = len(dependency_chain) / max(1, total_lines)
            if dependency_ratio > 0.5:  # More than 50% of lines affected
                return False
        except:
            return False  # Safe fallback if dependency analysis fails
        
        return True

    def _try_selective_evaluation(self):
        """Attempt selective evaluation with safe fallback"""
        try:
            current_text = self.editor.toPlainText()
            changed_lines = self.detect_changed_lines(self._last_evaluation_text, current_text)
            
            if not changed_lines:
                return True  # No changes needed
            
            # Attempt selective evaluation
            success = self.evaluate_changed_lines_only(changed_lines)
            if success:
                # Update last evaluation text on success
                self._last_evaluation_text = current_text
                return True
            
        except Exception as e:
            # Log error but fall back gracefully
            if hasattr(self.editor, '_debug_enabled') and self.editor._debug_enabled:
                print(f"Selective evaluation failed: {e}")
        
        return False  # Fall back to full evaluation

    def evaluate_changed_lines_only(self, changed_lines):
        """Stage 3.2/3.3: Evaluate only changed lines and their dependents with dependency-aware caching"""
        try:
            # Stage 3.3: Invalidate dependency cache for changed lines first
            lines_to_evaluate = self.invalidate_dependency_cache(changed_lines)
            
            # Get current content
            lines = self.editor.toPlainText().split('\n')
            doc = self.results.document()

            # Check if all content has been deleted (all lines are empty)
            all_empty = all(not line.strip() for line in lines)
            if all_empty:
                # Clear all caches when all content is deleted to prevent stale values
                self._expression_cache.clear()
                self.raw_values.clear()
                self._line_result_cache.clear()
                self._dependency_fingerprints.clear()
                if hasattr(self.editor, 'ln_value_map'):
                    self.editor.ln_value_map.clear()
                # Set results to empty and return early
                self.results.setPlainText('')
                return True
            
            # Store current cursor and scroll positions
            current_cursor = self.editor.textCursor()
            current_cursor_position = current_cursor.position()
            editor_scroll = self.editor.verticalScrollBar().value()
            results_scroll = self.results.verticalScrollBar().value()
            
            # Preserve existing results for unchanged lines
            out = [''] * len(lines)
            for i in range(min(len(lines), doc.blockCount())):
                block = doc.findBlockByNumber(i)
                if block.isValid():
                    out[i] = block.text()
            
            # Initialize evaluation context with existing LN values
            vals = {}
            
            # First pass: populate vals with existing LN values from cached results
            for i in range(len(lines)):
                if i not in lines_to_evaluate:
                    # If this line has a cached result, use it
                    if i in self._line_result_cache:
                        cached_entry = self._line_result_cache[i]
                        vals[f'LN{i+1}'] = cached_entry['result']
                        continue
                        
                    # Otherwise try older cache mechanism
                    cached_result = self.get_cached_result(lines[i].strip(), i+1)
                    if cached_result and cached_result['result'] is not None:
                        vals[f'LN{i+1}'] = cached_result['result']
            
            # Second pass: evaluate only the lines that need updating
            for i in sorted(lines_to_evaluate):
                if i >= len(lines):
                    continue
                    
                line = lines[i].strip()
                if not line or line.startswith('//'):
                    # Empty line - explicitly clear any previous result or error message
                    out[i] = ''
                    
                    # Clear all caches for this line
                    if i in self._line_result_cache:
                        self._line_result_cache.pop(i, None)
                    if i in self._dependency_fingerprints:
                        self._dependency_fingerprints.pop(i, None)
                    if i+1 in self.raw_values:  # i+1 because raw_values uses 1-based indexing
                        self.raw_values.pop(i+1, None)
                    
                    # Mark this as a line that needs updating in the results display
                    lines_to_evaluate.add(i)
                    
                    continue
                
                try:
                    # Stage 3.3: Track dependencies during evaluation
                    dependencies = {}
                    
                    # Find all LN references in this line to track dependencies
                    ln_matches = list(re.finditer(r'\bLN(\d+)\b', line, re.IGNORECASE))
                    for match in ln_matches:
                        ln_num = int(match.group(1)) - 1  # Convert to 0-based index
                        if 0 <= ln_num < len(lines):
                            dependencies[ln_num] = lines[ln_num]
                    
                    # Try the dependency-aware cache first
                    if i in self._line_result_cache and i not in changed_lines:
                        cache_entry = self._line_result_cache[i]
                        result = cache_entry['result']
                        out[i] = cache_entry['formatted_result']
                    else:
                        # Evaluate the line
                        processed_line = self.editor.process_ln_refs(line)
                        result = eval(processed_line, globals(), vals)
                        
                        # Format the result for display
                        formatted_result = self._format_result(result)
                        out[i] = formatted_result
                        
                        # Stage 3.3: Cache result with dependency information
                        self.cache_line_result_with_dependencies(i, line, result, dependencies)
                    
                    # Store LN value for other lines to reference
                    vals[f'LN{i+1}'] = result
                    
                except Exception as e:
                    err_msg = str(e)
                    if 'invalid syntax' in err_msg:
                        err_msg = 'Syntax error'
                    out[i] = f"Error: {err_msg}"
                    vals[f'LN{i+1}'] = None  # Default value to prevent cascade errors
                    
                    # Clear any cached results for this line on error
                    if i in self._line_result_cache:
                        self._line_result_cache.pop(i, None)
            
            # Update results display
            cursor = QTextCursor(doc)
            cursor.beginEditBlock()
            
            # Ensure all changed lines that are now empty have their results cleared
            # This handles both deleted lines and lines that were backspaced to empty
            old_line_count = doc.blockCount()
            new_line_count = len(lines)
            
            # First handle completely deleted lines
            if old_line_count > new_line_count:
                for i in range(new_line_count, old_line_count):
                    # Add these to the lines_to_evaluate set to ensure they get cleared
                    lines_to_evaluate.add(i)
                    # Make sure there's a blank entry in the output list
                    if i >= len(out):
                        out.extend([''] * (i - len(out) + 1))
                    out[i] = ''
                    
            # Also handle lines that were changed to become empty but still exist
            for i in changed_lines:
                if i < len(lines) and not lines[i].strip():
                    # This line was changed and is now empty - ensure it's in lines_to_evaluate
                    lines_to_evaluate.add(i)
                    # Explicitly set output to empty
                    out[i] = ''
                    # Clear any cached values
                    if i in self._line_result_cache:
                        self._line_result_cache.pop(i, None)
            
            # Enhanced fix: Apply the same protection here as in the main evaluation
            current_editor_text = self.editor.toPlainText().strip()

            # Case 1: Single empty line
            if len(lines) == 1 and len(current_editor_text) == 0:
                print(f"DEBUG: Single empty line detected in selective evaluation - clearing results")
                out = ['']
            # Case 2: Empty editor but many results (condensed results from other tabs)
            elif len(current_editor_text) == 0 and len(out) > 5:
                print(f"DEBUG: Empty editor with {len(out)} results detected in selective evaluation - clearing condensed results")
                out = [''] * len(lines)

            # Update only the affected lines to minimize UI disruption
            for i in lines_to_evaluate:
                if i < len(out):
                    # Move to the specific line
                    cursor.movePosition(QTextCursor.Start)
                    cursor.movePosition(QTextCursor.Down, QTextCursor.MoveAnchor, i)
                    cursor.select(QTextCursor.LineUnderCursor)
                    cursor.insertText(out[i])
            
            cursor.endEditBlock()
            
            # Restore cursor and scroll positions
            restored_cursor = QTextCursor(self.editor.document())
            restored_cursor.setPosition(current_cursor_position)
            self.editor.setTextCursor(restored_cursor)
            
            self._syncing_scroll = True
            self.editor.verticalScrollBar().setValue(editor_scroll)
            self.results.verticalScrollBar().setValue(results_scroll)
            self._syncing_scroll = False
            
            # Force highlighting update
            self.editor.highlightCurrentLine()
            QTimer.singleShot(10, lambda: self._force_sync_from_editor())
            
            return True
            
        except Exception as e:
            if hasattr(self.editor, '_debug_enabled') and self.editor._debug_enabled:
                print(f"Selective evaluation error: {e}")
            return False
            
    # Stage 3.3: Dependency-Aware Caching System methods
    def cache_line_result_with_dependencies(self, line_number, content, result, dependencies):
        """Cache result with its dependency fingerprint
        
        Args:
            line_number: The line number being cached
            content: The source content of the line
            result: The evaluated result
            dependencies: Dict of {line_num: content} for all lines this result depends on
        """
        # Create content hash for quick change detection
        content_hash = hash(content)
        
        # Create dependency fingerprint by hashing each dependency's content
        dependency_fingerprint = {}
        for dep_line, dep_content in dependencies.items():
            dependency_fingerprint[dep_line] = hash(dep_content)
        
        # Store the cache entry
        self._line_result_cache[line_number] = {
            'result': result,
            'formatted_result': self._format_result(result),
            'dependencies': dependency_fingerprint,
            'hash': content_hash
        }
        
        # Update result dependencies for efficient invalidation
        for dep_line in dependencies.keys():
            if dep_line not in self._result_dependencies:
                self._result_dependencies[dep_line] = set()
            self._result_dependencies[dep_line].add(line_number)
        
        # Store fingerprints separately for easy checking
        self._dependency_fingerprints[line_number] = dependency_fingerprint
    
    def update_dependent_lines(self, changed_line, new_value):
        """Efficiently propagate changes to dependent lines
        
        Args:
            changed_line: The line that was changed
            new_value: The new value of the changed line
        """
        # If no one depends on this line, exit early
        if changed_line not in self._result_dependencies:
            return set()
        
        # Get all lines that depend directly on this line's result
        dependent_lines = self._result_dependencies[changed_line].copy()
        
        # For batch processing, collect all lines that need updating
        lines_to_update = set()
        
        # For each dependent line, check if we can use the cached result
        for dep_line in dependent_lines:
            # If the line isn't in the cache, it will be recalculated
            if dep_line not in self._line_result_cache:
                lines_to_update.add(dep_line)
                continue
                
            # Get the cached entry
            cache_entry = self._line_result_cache[dep_line]
            dependency_fingerprint = cache_entry['dependencies']
            
            # If the dependency fingerprint doesn't include this line,
            # this means the dependency structure has changed - force recalculation
            if changed_line not in dependency_fingerprint:
                self._line_result_cache.pop(dep_line, None)
                lines_to_update.add(dep_line)
                continue
                
            # Mark this line for recalculation by removing from cache
            self._line_result_cache.pop(dep_line, None)
            lines_to_update.add(dep_line)
        
        return lines_to_update
    
    def invalidate_dependency_cache(self, changed_lines):
        """Invalidate only relevant cache entries
        
        Args:
            changed_lines: Set of line numbers that have changed
            
        Returns:
            Set of line numbers that need to be re-evaluated
        """
        # For each changed line, find lines that depend on it and invalidate them
        affected_lines = set()
        
        for line in changed_lines:
            # First, directly invalidate the changed line itself
            if line in self._line_result_cache:
                self._line_result_cache.pop(line, None)
            
            # Then find and invalidate all dependent lines
            if line in self._result_dependencies:
                dependents = self._result_dependencies[line].copy()
                affected_lines.update(dependents)
                
                # Recursively find all indirect dependents too
                more_dependents = set(dependents)
                while more_dependents:
                    current = more_dependents.pop()
                    # Get dependents of this line if any
                    if current in self._result_dependencies:
                        next_level = self._result_dependencies[current] - affected_lines
                        more_dependents.update(next_level)
                        affected_lines.update(next_level)
        
        # Invalidate all affected lines' cache entries
        for line in affected_lines:
            self._line_result_cache.pop(line, None)
            
        # Return full set of lines that need re-evaluation
        return affected_lines.union(changed_lines)

    def _initialize_evaluation(self):
        """Initialize evaluation state and prepare data structures"""
        evaluation_context = {}
        
        # Debug and performance logging setup
        if hasattr(self.editor, '_debug_enabled') and self.editor._debug_enabled:
            start_time = self.editor._log_perf("evaluate")
            print(f"EVALUATION TRIGGERED at line {self.editor.textCursor().blockNumber()}")
            
            # Add stack trace to see what's calling evaluation
            stack = traceback.extract_stack()
            print("EVAL CALL STACK:")
            for frame in stack[-5:-1]:  # Show last 4 frames before this one
                print(f"  {frame.filename}:{frame.lineno} in {frame.name}()")
        
        # Store current cursor and scroll positions
        evaluation_context['cursor'] = self.editor.textCursor()
        evaluation_context['editor_scroll'] = self.editor.verticalScrollBar().value()
        evaluation_context['results_scroll'] = self.results.verticalScrollBar().value()
        
        # Check if this is a cursor-triggered evaluation
        evaluation_context['is_cursor_triggered'] = hasattr(self.editor, '_cursor_triggered_eval')
        if evaluation_context['is_cursor_triggered']:
            delattr(self.editor, '_cursor_triggered_eval')
        
        # Ensure line IDs are properly assigned
        self.editor.reassign_line_ids()
        
        # Initialize data structures
        evaluation_context['lines'] = self.editor.toPlainText().split("\n")
        evaluation_context['vals'] = [None] * len(evaluation_context['lines'])
        self.editor.ln_value_map = {}
        
        # Stage 2 Optimization: Clear LN reference cache when values change
        self.editor.clear_ln_reference_cache()
        
        id_map = {}
        evaluation_context['doc'] = self.editor.document()
        
        # Update separator lines
        self.editor.update_separator_lines()
        
        # Build id_map and initialize ln_value_map
        for i in range(evaluation_context['doc'].blockCount()):
            blk = evaluation_context['doc'].findBlockByNumber(i)
            d = blk.userData()
            if isinstance(d, LineData):
                id_map[d.id] = i
                # Initialize with None to ensure the ID exists in the map
                self.editor.ln_value_map[d.id] = None
        
        evaluation_context['id_map'] = id_map
        return evaluation_context

    def _finalize_evaluation(self, out, evaluation_context):
        """Finalize evaluation and update UI"""
        # Ensure all empty lines have empty results
        lines = self.editor.toPlainText().split('\n')

        # Check if all content has been deleted (all lines are empty)
        all_empty = all(not line.strip() for line in lines)
        if all_empty:
            # Clear all caches when all content is deleted to prevent stale values
            self._expression_cache.clear()
            self.raw_values.clear()
            self._line_result_cache.clear()
            self._dependency_fingerprints.clear()
            if hasattr(self.editor, 'ln_value_map'):
                self.editor.ln_value_map.clear()
            # Also ensure the out array only contains empty strings for all empty lines
            out = [''] * len(lines)

        for i, line in enumerate(lines):
            if not line.strip() and i < len(out):
                out[i] = ''
                # Also clear all caches for empty lines
                if i in self._line_result_cache:
                    self._line_result_cache.pop(i, None)
                if i in self._dependency_fingerprints:
                    self._dependency_fingerprints.pop(i, None)
                if i+1 in self.raw_values:  # i+1 because raw_values uses 1-based indexing
                    self.raw_values.pop(i+1, None)
        
        # Make sure output array is not longer than the current document
        if len(out) > len(lines):
            out = out[:len(lines)]
        
        # Enhanced fix: If the current tab is empty but we're trying to set many results, clear them
        current_editor_text = self.editor.toPlainText().strip()

        # Case 1: Single empty line
        if len(lines) == 1 and len(current_editor_text) == 0:
            print(f"DEBUG: Single empty line detected - clearing results")
            text_content = ""
        # Case 2: Empty editor but many results (condensed results from other tabs)
        elif len(current_editor_text) == 0 and len(out) > 5:
            print(f"DEBUG: Empty editor with {len(out)} results detected - clearing condensed results")
            text_content = ""
        else:
            # Update results with plain text (no HTML needed since we're using QPlainTextEdit)
            text_content = '\n'.join(out)

        self.results.setPlainText(text_content)

        # Clear mass delete flag after evaluation is complete (with delay to prevent premature clearing)
        calculator = None
        parent = self.parent()
        while parent and not hasattr(parent, 'tabs'):
            parent = parent.parent()
        if parent and hasattr(parent, 'tabs'):
            calculator = parent
            if hasattr(calculator, '_mass_delete_in_progress') and calculator._mass_delete_in_progress:
                # Use a timer to clear the flag after a short delay to allow all related evaluations to complete
                def clear_mass_delete_flag():
                    if hasattr(calculator, '_mass_delete_in_progress'):
                        print(f"DEBUG: Mass delete flag CLEARED from True to False (delayed)")
                        calculator._mass_delete_in_progress = False

                # Clear the flag after 1000ms to allow all cascading evaluations to be blocked
                QTimer.singleShot(1000, clear_mass_delete_flag)

        # Stage 3.1: Build/update line dependency graph after evaluation
        # Only build if dirty or if this is a major evaluation
        if self._dependency_graph_dirty or not hasattr(self, '_last_dependency_build'):
            self.build_line_dependencies()
            self._last_dependency_build = time.time()
        
        # Restore cursor and synchronized scroll position
        self.editor.setTextCursor(evaluation_context['cursor'])
        # Only set one scrollbar - the synchronization will handle the other
        self._syncing_scroll = True  # Temporarily disable sync to avoid double-setting
        self.editor.verticalScrollBar().setValue(evaluation_context['editor_scroll'])
        self._syncing_scroll = False
        
        # Force a sync after content update to ensure both are aligned
        QTimer.singleShot(10, lambda: self._force_sync_from_editor())
        
        # Force highlight update if this was cursor-triggered
        if evaluation_context['is_cursor_triggered']:
            self.editor.highlightCurrentLine()

    def _force_sync_from_editor(self):
        """Force synchronization from editor to results - used after content updates"""
        if not self._syncing_scroll:
            current_value = self.editor.verticalScrollBar().value()
            self._sync_editor_to_results(current_value)

    def wheelEvent(self, event):
        if event.modifiers() & Qt.ControlModifier:
            # Forward the wheel event to the editor
            self.editor.wheelEvent(event)
        else:
            super().wheelEvent(event)

    def mousePressEvent(self, event):
        if event.button() == Qt.MiddleButton and event.modifiers() & Qt.ControlModifier:
            # Forward the middle click event to the editor
            self.editor.mousePressEvent(event)
        else:
            super().mousePressEvent(event)

    def _end_navigation(self):
        """Mark end of navigation period"""
        self._is_navigating = False

    def _start_navigation(self):
        """Mark start of navigation period"""
        self._is_navigating = True
        self._navigation_timer.start()

    def updateResultsLineNumberAreaWidth(self, _):
        """Update the results line number area width and layout"""
        line_number_width = self.results_lnr.lineNumberAreaWidth()
        self.results.setViewportMargins(line_number_width + 6, 0, 0, 0)

    def updateResultsLineNumberArea(self, rect, dy):
        """Update the results line number area when scrolled"""
        if dy:
            self.results_lnr.scroll(0, dy)
        else:
            self.results_lnr.update(0, rect.y(), self.results_lnr.width(), rect.height())
        if rect.contains(self.results.viewport().rect()):
            self.updateResultsLineNumberAreaWidth(0)

    def resizeResultsLineNumberArea(self):
        """Handle resize events for the results line number area - matches editor approach"""
        if hasattr(self, 'results_lnr'):
            cr = self.results.contentsRect()
            self.results_lnr.setGeometry(cr.x(), cr.y(), self.results_lnr.lineNumberAreaWidth(), cr.height())

    def setupResultsLayout(self):
        """Setup the layout for the results container with line numbers"""
        # Simply add the results widget to the container
        if hasattr(self, 'results_container') and hasattr(self, 'results'):
            self.results.setParent(self.results_container)
            self.results.setGeometry(0, 0, self.results_container.width(), self.results_container.height())

    def resizeResultsContainer(self):
        """Handle resize events for the results container"""
        if hasattr(self, 'results_container') and hasattr(self, 'results'):
            container_size = self.results_container.size()
            # Update results widget size to fill container
            self.results.setGeometry(0, 0, container_size.width(), container_size.height())
            # The line number area will be repositioned by the resize event of the results widget
            
    def check_and_fix_results(self):
        """Post-evaluation check: Implement the simple fix suggested by user"""
        try:
            current_editor_text = self.editor.toPlainText().strip()
            lines = self.editor.toPlainText().split('\n')
            current_results = self.results.toPlainText().strip()

            print(f"DEBUG: Post-evaluation check - Editor lines: {len(lines)}, Editor empty: {len(current_editor_text) == 0}, Has results: {len(current_results) > 0}")

            # Simple fix: If there is only 1 line and the expression field is empty, clear line 1 results
            if len(lines) == 1 and len(current_editor_text) == 0 and len(current_results) > 0:
                print(f"DEBUG: Post-evaluation fix - Single empty line with results detected, clearing results")
                # Directly clear the results without triggering more evaluations
                self.results.blockSignals(True)
                self.results.setPlainText("")
                self.results.blockSignals(False)
            # Extended fix: If editor is completely empty but has many results, clear them
            elif len(current_editor_text) == 0 and len(current_results.split('\n')) > 5:
                print(f"DEBUG: Post-evaluation fix - Empty editor with many results detected, clearing results")
                self.results.blockSignals(True)
                self.results.setPlainText("")
                self.results.blockSignals(False)
        except Exception as e:
            print(f"DEBUG: Error in check_and_fix_results: {e}")

    def efficient_brute_force_fix(self):
        """Efficient brute force fix - runs continuously with 300ms interval"""
        try:
            current_editor_text = self.editor.toPlainText().strip()
            lines = self.editor.toPlainText().split('\n')
            current_results = self.results.toPlainText()
            result_lines = current_results.split('\n')

            # Case 1: Single empty line (original fix)
            if len(lines) == 1 and len(current_editor_text) == 0 and len(current_results.strip()) > 0:
                print(f"DEBUG: Efficient brute force fix - Single empty line with results detected, clearing results")
                self.results.blockSignals(True)
                self.results.setPlainText("")
                self.results.blockSignals(False)
                return

            # Case 2: Check for condensed results in individual lines
            fixed_results = []
            results_were_fixed = False

            for i, line in enumerate(lines):
                line_content = line.strip()

                if i < len(result_lines):
                    result_content = result_lines[i].strip()

                    # If the editor line is empty but the result line has content, check if it's condensed
                    if len(line_content) == 0 and len(result_content) > 0:
                        # Check if this looks like condensed results - be more aggressive
                        # Look for: commas, multiple numbers, or very long results on empty lines
                        is_condensed = (
                            ',' in result_content or  # Multiple values separated by commas
                            len(result_content) > 15 or  # Very long results are likely condensed
                            result_content.count('.') > 1  # Multiple decimal numbers
                        )

                        if is_condensed:
                            print(f"DEBUG: Condensed results detected on line {i+1}: '{result_content[:50]}...'")
                            fixed_results.append("")  # Clear the condensed results
                            results_were_fixed = True
                        else:
                            fixed_results.append(result_content)  # Keep normal single results
                    else:
                        fixed_results.append(result_content)  # Keep as-is
                else:
                    fixed_results.append("")  # Add empty result for new lines

            # Apply the fix if we found condensed results
            if results_were_fixed:
                print(f"DEBUG: Applying condensed results fix")
                self.results.blockSignals(True)
                self.results.setPlainText('\n'.join(fixed_results))
                self.results.blockSignals(False)

        except Exception as e:
            # Silently ignore errors to avoid spam
            pass

    def clear_results_for_empty_lines(self, changed_lines):
        """Force clear results for lines that are now empty or removed"""
        if not changed_lines:
            return

        # Get current lines
        lines = self.editor.toPlainText().split('\n')
        doc = self.results.document()

        # NOTE: Line count synchronization is now handled by on_editor_block_count_changed()
        # This method now only handles clearing results for empty lines, not line count sync
        
        # Check each changed line
        for line_idx in changed_lines:
            # Skip if beyond available lines
            if line_idx >= len(lines):
                continue
                
            # If the line is empty, clear its result
            if not lines[line_idx].strip():
                # Get the results document
                
                # Make sure the line exists in results
                if line_idx < doc.blockCount():
                    # Create a cursor to modify the specific line
                    cursor = QTextCursor(doc)
                    cursor.beginEditBlock()
                    cursor.movePosition(QTextCursor.Start)
                    cursor.movePosition(QTextCursor.Down, QTextCursor.MoveAnchor, line_idx)
                    cursor.select(QTextCursor.LineUnderCursor)
                    # We need to insert empty text rather than just removing, to ensure it updates
                    cursor.insertText("")
                    cursor.endEditBlock()
                    
                    # Force update
                    self.results.setTextCursor(cursor)
                
                # Clear all caches for this line
                if line_idx in self._line_result_cache:
                    self._line_result_cache.pop(line_idx, None)
                if line_idx in self._dependency_fingerprints:
                    self._dependency_fingerprints.pop(line_idx, None)
                if line_idx+1 in self.raw_values:  # raw_values uses 1-based indexing
                    self.raw_values.pop(line_idx+1, None)

    def _evaluate_lines_loop(self, lines, vals, doc):
        """Main line-by-line evaluation logic with Stage 1 caching optimization"""
        out = []

        # Check if all lines are empty - if so, clear all caches and return empty results
        all_lines_empty = all(not line.strip() for line in lines)

        if all_lines_empty:
            # Clear all caches when all content is empty
            self._expression_cache.clear()
            self.raw_values.clear()
            self._line_result_cache.clear()
            self._dependency_fingerprints.clear()
            if hasattr(self.editor, 'ln_value_map'):
                self.editor.ln_value_map.clear()
            # Return empty results for all lines
            return [''] * len(lines)

        # Tab switching optimization - Stage 1: Detect cross-sheet references during evaluation
        detected_cross_sheet_refs = False
        
        # First, clear any existing results beyond the current line count
        # This handles deleted lines
        old_block_count = doc.blockCount()
        if old_block_count > len(lines):
            for i in range(len(lines), old_block_count):
                if i < old_block_count:
                    cursor = QTextCursor(doc)
                    cursor.movePosition(QTextCursor.Start)
                    cursor.movePosition(QTextCursor.Down, QTextCursor.MoveAnchor, i)
                    cursor.select(QTextCursor.LineUnderCursor)
                    cursor.insertText("")
        
        # Evaluate each line
        for idx, line in enumerate(lines):
            self.current_line = line  # Store current line for context
            s = line.strip()
            if not s:  # Empty line
                vals[idx] = None
                blk = doc.findBlockByNumber(idx)
                data = blk.userData()
                if isinstance(data, LineData):
                    self.editor.ln_value_map[data.id] = vals[idx]
                    
                # Important: Make sure to clear any previous cached values
                if idx+1 in self.raw_values:  # raw_values uses 1-based indexing
                    self.raw_values.pop(idx+1, None)
                    
                # Clear the result in this line    
                out.append("")
                continue
            
            if s.startswith(":::"):  # Comment line
                vals[idx] = None
                blk = doc.findBlockByNumber(idx)
                data = blk.userData()
                if isinstance(data, LineData):
                    self.editor.ln_value_map[data.id] = vals[idx]
                out.append("")
                continue

            # Stage 1: Check for cached result first (skip for LN references and cross-sheet refs)
            has_references = (re.search(r"\b(?:s\.|S\.)?(?:ln|LN)\d+\b", s, re.IGNORECASE) or
                            re.search(r'\bS\.[^.]+\.LN\d+\b', s, re.IGNORECASE))
            
            if not has_references:
                cached_result = self.get_cached_result(s, idx + 1)
                if cached_result is not None:
                    # Use cached result
                    out.append(cached_result)
                    # We need to also set vals[idx] for potential LN references
                    # Extract the raw value from cache for vals
                    if idx + 1 in self.raw_values:
                        vals[idx] = self.raw_values[idx + 1]
                    else:
                        # Try to parse numeric value from cached result
                        try:
                            if isinstance(cached_result, str) and cached_result.replace(',', '').replace('.', '').replace('-', '').isdigit():
                                vals[idx] = float(cached_result.replace(',', ''))
                            else:
                                vals[idx] = cached_result
                        except:
                            vals[idx] = cached_result
                    
                    # Update LN value map
                    blk = doc.findBlockByNumber(idx)
                    data = blk.userData()
                    if isinstance(data, LineData):
                        self.editor.ln_value_map[data.id] = vals[idx]
                    continue

            # Tab switching optimization - Check for cross-sheet references in this line
            if re.search(r'\bS\.[^.]+\.LN\d+\b', s, re.IGNORECASE):
                detected_cross_sheet_refs = True

            # Try special cases first
            try:
                # Get the current block and its ID
                blk = doc.findBlockByNumber(idx)
                data = blk.userData()
                current_id = data.id if isinstance(data, LineData) else None

                # Pre-process the expression to handle padded numbers
                s = self._preprocess_expression(s)

                # Check for D() function call first
                d_func_match = re.match(r'D\((.*?)\)', s)
                if d_func_match:
                    # Extract the content inside D() and process it directly
                    date_content = d_func_match.group(1)
                    date_result = handle_date_arithmetic(date_content)
                    
                    if date_result is not None:
                        vals[idx] = date_result
                        if current_id:
                            self.editor.ln_value_map[current_id] = vals[idx]
                        formatted_result = self.format_number_for_display(date_result, idx + 1)
                        out.append(formatted_result)
                        # Stage 1: Cache the result (if no references)
                        if not has_references:
                            self.cache_evaluation_result(line.strip(), date_result, formatted_result, idx + 1)
                        continue

                # Check for unit conversion
                unit_result = self._handle_unit_conversion(s)
                if unit_result is not None:
                    vals[idx] = unit_result
                    if current_id:
                        self.editor.ln_value_map[current_id] = vals[idx]
                    formatted_result = self.format_number_for_display(unit_result, idx + 1)
                    out.append(formatted_result)
                    # Stage 1: Cache the result (if no references)
                    if not has_references:
                        self.cache_evaluation_result(line.strip(), unit_result, formatted_result, idx + 1)
                    continue

                # Check for currency conversion
                currency_result = handle_currency_conversion(s)
                if currency_result is not None:
                    vals[idx] = currency_result
                    if current_id:
                        self.editor.ln_value_map[current_id] = vals[idx]
                    formatted_result = self.format_number_for_display(currency_result, idx + 1)
                    out.append(formatted_result)
                    # Stage 1: Cache the result (if no references)
                    if not has_references:
                        self.cache_evaluation_result(line.strip(), currency_result, formatted_result, idx + 1)
                    continue

                # Check for truncate function call (both truncate and TR)
                trunc_match = re.match(r'(?:truncate|TR)\((.*?),(.*?)\)', s)
                if trunc_match:
                    # First evaluate the expression
                    expr = self.editor.process_ln_refs(trunc_match.group(1).strip())
                    decimals = int(eval(trunc_match.group(2).strip(), {"truncate": truncate, "TR": truncate, **GLOBALS}, {}))
                    
                    # Try unit conversion first
                    unit_result = self._handle_unit_conversion(expr)
                    if unit_result is not None:
                        v = truncate(unit_result, decimals)
                    else:
                        # Try currency conversion
                        currency_result = handle_currency_conversion(expr)
                        if currency_result is not None:
                            v = truncate(currency_result, decimals)
                        else:
                            # If not a unit or currency conversion, evaluate as regular expression
                            val = eval(expr, {"truncate": truncate, "TR": truncate, **GLOBALS}, {})
                            v = truncate(val, decimals)
                        
                    vals[idx] = v
                    if current_id:
                        self.editor.ln_value_map[current_id] = vals[idx]
                    out.append(self.format_number_for_display(v, idx + 1))
                    continue

                # Try special commands
                cmd_result = self._handle_special_commands(s, idx, lines, vals)
                if cmd_result is not None:
                    vals[idx] = cmd_result
                    if current_id:
                        self.editor.ln_value_map[current_id] = vals[idx]
                    out.append(self.format_number_for_display(cmd_result, idx + 1))
                    continue

                # Process LN references if present
                if re.search(r"\b(?:s\.|S\.)?(?:ln|LN)\d+\b", s, re.IGNORECASE):
                    s = self.editor.process_ln_refs(s)
                    # print(f"Line {idx + 1} after processing refs: {s}")  # Debug print - commented for performance

                # Try to evaluate the expression with math functions
                v = eval(s, {"truncate": truncate, "mean": statistics.mean, "TR": truncate, **GLOBALS}, {})
                vals[idx] = v
                if current_id:
                    self.editor.ln_value_map[current_id] = vals[idx]
                    # print(f"Stored value {v} for line ID {current_id}")  # Debug print - commented for performance
                
                # Format the output
                formatted_result = self.format_number_for_display(v, idx + 1)
                out.append(formatted_result)
                
                # Stage 1: Cache the result (if no references)
                if not has_references:
                    self.cache_evaluation_result(line.strip(), v, formatted_result, idx + 1)
            except TimecodeError as e:
                # Handle TimecodeError specifically to show the actual error message
                # print(f"Timecode error on line {idx + 1}: {str(e)}")  # Debug print - commented for performance
                vals[idx] = None
                if current_id:
                    self.editor.ln_value_map[current_id] = None
                out.append(f'TC ERROR: {str(e)}')
            except Exception as e:
                # print(f"Error evaluating line {idx + 1}: {str(e)}")  # Debug print - commented for performance
                vals[idx] = None
                if current_id:
                    self.editor.ln_value_map[current_id] = None
                out.append('ERROR!')
        
        # Tab switching optimization - Stage 1: Update cross-sheet reference flag
        self.has_cross_sheet_refs = detected_cross_sheet_refs
        
        return out

    def _handle_unit_conversion(self, expr):
        """Handle unit conversion expressions like '1 mile to km'"""
        pattern = r'(\d+(?:\.\d+)?)\s+(\w+)\s+to\s+(\w+)'
        match = re.match(pattern, expr.lower())
        if match:
            value, from_unit, to_unit = match.groups()
            # Handle unit abbreviations
            from_unit = UNIT_ABBR.get(from_unit.lower(), from_unit)
            to_unit = UNIT_ABBR.get(to_unit.lower(), to_unit)
            try:
                # Create quantity and convert
                q = ureg.Quantity(float(value), from_unit)
                result = q.to(to_unit)
                # Get the full spelling for display
                display_unit = UNIT_DISPLAY.get(to_unit, to_unit)
                # Return both the value and the unit
                return {'value': float(result.magnitude), 'unit': display_unit}
            except:
                return None
        return None

    # Stage 1 Performance Optimization Methods
    def _update_line_hashes(self):
        """Update cached hashes for all current lines"""
        import hashlib
        self._line_hashes = {}
        for i, line in enumerate(self._last_lines):
            # Create a hash of the line content for fast comparison
            line_hash = hashlib.md5(line.encode('utf-8')).hexdigest()
            self._line_hashes[i] = line_hash

    def detect_changed_lines(self, old_text, new_text):
        """Compare line by line to identify actual changes - Stage 1 optimization"""
        import hashlib
        
        old_lines = old_text.split('\n')
        new_lines = new_text.split('\n')
        changed_lines = set()
        
        # Check for content changes in existing lines
        max_lines = max(len(old_lines), len(new_lines))
        for i in range(max_lines):
            old_line = old_lines[i] if i < len(old_lines) else ""
            new_line = new_lines[i] if i < len(new_lines) else ""
            
            if old_line != new_line:
                changed_lines.add(i)
        
        # Update our tracking variables
        self._last_lines = new_lines
        self._changed_lines = changed_lines
        self._update_line_hashes()
        
        return changed_lines

    def analyze_change_type(self, changed_lines):
        """Analyze the type of changes to determine optimal evaluation strategy - Stage 1"""
        if not changed_lines:
            return 'none'
        
        # Sample a few changed lines to determine change type
        sample_lines = []
        for line_idx in list(changed_lines)[:3]:  # Sample up to 3 lines
            if line_idx < len(self._last_lines):
                sample_lines.append(self._last_lines[line_idx].strip())
        
        # Check if changes are only whitespace
        non_whitespace_changes = False
        for line in sample_lines:
            if line and not line.isspace():
                non_whitespace_changes = True
                break
        
        if not non_whitespace_changes:
            return 'whitespace'
        
        # Check for simple math expressions
        simple_math_pattern = r'^[0-9\s+\-*/().]+$'
        ln_pattern = r'\bLN\d+\b'
        cross_sheet_pattern = r'\bS\.[^.]+\.LN\d+\b'
        function_pattern = r'\b(?:TC|AR|truncate|mean|TR)\s*\('
        
        has_ln_refs = False
        has_cross_sheet = False
        has_functions = False
        has_simple_math = False
        
        for line in sample_lines:
            if not line:
                continue
                
            if re.search(cross_sheet_pattern, line, re.IGNORECASE):
                has_cross_sheet = True
            elif re.search(ln_pattern, line, re.IGNORECASE):
                has_ln_refs = True
            elif re.search(function_pattern, line, re.IGNORECASE):
                has_functions = True
            elif re.match(simple_math_pattern, line):
                has_simple_math = True
        
        # Return most complex type found
        if has_cross_sheet:
            return 'cross_sheet'
        elif has_ln_refs:
            return 'ln_reference'
        elif has_functions:
            return 'complex'
        elif has_simple_math:
            return 'simple_math'
        else:
            return 'complex'  # Default to complex for unknown patterns

    def start_smart_evaluation_timer(self, changed_lines):
        """Start evaluation timer with intelligent delay based on change type - Stage 1"""
        import time
        
        self._last_change_time = time.time()
        change_type = self.analyze_change_type(changed_lines)
        self._change_type = change_type
        
        # Set timer interval based on change type
        if change_type == 'none' or change_type == 'whitespace':
            # No evaluation needed for whitespace-only changes
            return
        elif change_type == 'simple_math':
            # Fast evaluation for simple math
            interval = 100  # 100ms for simple expressions
        elif change_type == 'ln_reference':
            # Medium delay for LN references
            interval = 200  # 200ms for LN references
        elif change_type == 'cross_sheet':
            # Longer delay for cross-sheet references
            interval = 400  # 400ms for cross-sheet refs
        else:  # complex or unknown
            # Standard delay for complex expressions
            interval = 300  # 300ms for complex expressions
        
        self.timer.setInterval(interval)
        self.timer.start()

    def get_cached_result(self, line_content, line_number):
        """Get cached evaluation result if available - Stage 1 optimization"""
        import hashlib
        
        if not line_content.strip():
            return None
            
        # Create hash for the line content
        line_hash = hashlib.md5(line_content.encode('utf-8')).hexdigest()
        
        # Check if we have a cached result
        if line_hash in self._expression_cache:
            cached_result, cached_formatted = self._expression_cache[line_hash]
            
            # Update raw values for cached results
            if isinstance(cached_result, (int, float)):
                self.raw_values[line_number] = cached_result
            
            return cached_formatted
        
        return None

    def cache_evaluation_result(self, line_content, result, formatted_result, line_number):
        """Cache evaluation result for future use - Stage 1 optimization"""
        import hashlib
        
        if not line_content.strip():
            return
            
        # Create hash for the line content
        line_hash = hashlib.md5(line_content.encode('utf-8')).hexdigest()
        
        # Manage cache size
        if len(self._expression_cache) >= self._cache_max_size:
            # Remove oldest entries (simple FIFO approach)
            keys_to_remove = list(self._expression_cache.keys())[:100]
            for key in keys_to_remove:
                del self._expression_cache[key]
        
        # Store the result
        self._expression_cache[line_hash] = (result, formatted_result)

    def should_skip_evaluation(self, changed_lines):
        """Determine if evaluation should be skipped - Stage 1 optimization"""
        if not changed_lines:
            return True
            
        # Skip if only whitespace changes
        change_type = self.analyze_change_type(changed_lines)
        if change_type == 'whitespace':
            return True
            
        return False

    def _sync_editor_to_results(self, value):
        """Sync results scrollbar when editor scrollbar changes"""
        if not self._syncing_scroll:
            start_time = None
            if hasattr(self.editor, '_log_perf'):
                start_time = self.editor._log_perf("_sync_editor_to_results")
                
            self._syncing_scroll = True
            try:
                # Get the editor's scrollbar
                editor_scrollbar = self.editor.verticalScrollBar()
                results_scrollbar = self.results.verticalScrollBar()
                
                # Calculate the scroll ratio to handle different content heights
                editor_max = max(1, editor_scrollbar.maximum())
                results_max = max(1, results_scrollbar.maximum())
                
                if editor_max > 0 and results_max > 0:
                    # Calculate proportional position
                    ratio = value / editor_max if editor_max > 0 else 0
                    target_value = int(ratio * results_max)
                    results_scrollbar.setValue(target_value)
                else:
                    # Direct value if no scaling needed
                    results_scrollbar.setValue(value)
            finally:
                self._syncing_scroll = False
                if start_time and hasattr(self.editor, '_log_perf'):
                    self.editor._log_perf("_sync_editor_to_results", start_time)

    # Stage 3.1: Line Dependency Graph Infrastructure
    def build_line_dependencies(self):
        """Analyze all lines to build dependency graph for internal LN references"""
        start_time = None
        if hasattr(self.editor, '_log_perf'):
            start_time = self.editor._log_perf("build_line_dependencies")
        
        # Clear existing dependencies
        self.line_dependencies.clear()
        self.line_references.clear()
        self.dependency_graph_cache.clear()
        self._dependency_chain_cache.clear()
        
        lines = self.editor.toPlainText().split('\n')
        
        for line_idx, line_content in enumerate(lines):
            line_number = line_idx + 1  # 1-based line numbers
            
            # Find all internal LN references in this line (exclude cross-sheet references)
            internal_refs = self._find_internal_ln_references(line_content)
            
            if internal_refs:
                # Store what this line references
                self.line_references[line_number] = internal_refs
                
                # Update bidirectional mapping - for each referenced line, 
                # note that current line depends on it
                for ref_line in internal_refs:
                    if ref_line not in self.line_dependencies:
                        self.line_dependencies[ref_line] = set()
                    self.line_dependencies[ref_line].add(line_number)
        
        self._dependency_graph_dirty = False
        
        if start_time and hasattr(self.editor, '_log_perf'):
            self.editor._log_perf("build_line_dependencies", start_time)
    
    def _find_internal_ln_references(self, line_content):
        """Find internal LN references in a line, excluding cross-sheet references"""
        # Use pre-compiled regex for performance
        matches = self._internal_ln_pattern.findall(line_content)
        
        # Filter out any that are part of cross-sheet references (S.Sheet.LN)
        # by checking if LN is preceded by a sheet reference pattern
        internal_refs = set()
        
        for match in matches:
            ln_number = int(match)
            # Check if this LN reference is part of a cross-sheet reference
            ln_pos = line_content.find(f'LN{ln_number}')
            if ln_pos > 0:
                # Look backward to see if there's a sheet reference (S.SheetName.)
                prefix = line_content[:ln_pos]
                # Check if it ends with S.SheetName. pattern
                if re.search(r'S\.[^.]+\.$', prefix):
                    continue  # Skip cross-sheet references
            
            internal_refs.add(ln_number)
        
        return internal_refs
    
    def update_line_dependencies(self, line_number, old_content, new_content):
        """Update dependencies when a single line changes"""
        if self._dependency_graph_dirty:
            # If graph is dirty, rebuild entirely
            return self.build_line_dependencies()
        
        start_time = None
        if hasattr(self.editor, '_log_perf'):
            start_time = self.editor._log_perf("update_line_dependencies")
        
        # Remove old dependencies for this line
        old_refs = self.line_references.get(line_number, set())
        for old_ref in old_refs:
            if old_ref in self.line_dependencies:
                self.line_dependencies[old_ref].discard(line_number)
                if not self.line_dependencies[old_ref]:
                    del self.line_dependencies[old_ref]
        
        # Find new references
        new_refs = self._find_internal_ln_references(new_content)
        
        # Update line_references
        if new_refs:
            self.line_references[line_number] = new_refs
        else:
            self.line_references.pop(line_number, None)
        
        # Add new dependencies
        for new_ref in new_refs:
            if new_ref not in self.line_dependencies:
                self.line_dependencies[new_ref] = set()
            self.line_dependencies[new_ref].add(line_number)
        
        # Clear caches that might be affected
        self.dependency_graph_cache.clear()
        self._dependency_chain_cache.clear()
        
        if start_time and hasattr(self.editor, '_log_perf'):
            self.editor._log_perf("update_line_dependencies", start_time)
    
    def get_dependent_lines(self, line_number):
        """Get all lines that depend on the given line (cached)"""
        if line_number in self.dependency_graph_cache:
            return self.dependency_graph_cache[line_number]
        
        # Get direct dependents
        direct_dependents = self.line_dependencies.get(line_number, set()).copy()
        
        # Cache and return
        self.dependency_graph_cache[line_number] = direct_dependents
        return direct_dependents
    
    def get_dependency_chain(self, changed_lines):
        """Get the complete chain of lines that need to be re-evaluated due to changes"""
        # Quick path - if dependency graph is empty, we evaluate all lines
        if not self.line_dependencies:
            return set(range(0, self.editor.document().blockCount()))
            
        # Check if we have this result cached
        cache_key = frozenset(changed_lines)
        if cache_key in self._dependency_chain_cache:
            return self._dependency_chain_cache[cache_key]
        
        # Start with the changed lines themselves
        affected_lines = set(changed_lines)
        
        # Use breadth-first search to find all affected lines
        queue = list(changed_lines)
        visited = set(changed_lines)
        
        while queue:
            line = queue.pop(0)
            # Get all lines that depend on this line
            if line in self.line_dependencies:
                dependents = self.line_dependencies[line]
                for dep in dependents:
                    if dep not in visited:
                        visited.add(dep)
                        queue.append(dep)
                        affected_lines.add(dep)
            
            iteration_count += 1
        
        # Cache the result
        result = frozenset(affected_lines)
        self._dependency_chain_cache[changed_set] = result
        
        if start_time and hasattr(self.editor, '_log_perf'):
            self.editor._log_perf("get_dependency_chain", start_time)
        
        return result
    
    def clear_dependency_caches(self):
        """Clear dependency caches when major changes occur"""
        self.dependency_graph_cache.clear()
        self._dependency_chain_cache.clear()
        self._dependency_graph_dirty = True

    def evaluate_cross_sheet_lines_only(self):
        """Stage 2 optimization: Only evaluate lines with cross-sheet references"""
        if not hasattr(self, 'has_cross_sheet_refs') or not self.has_cross_sheet_refs:
            return

        # Skip cross-sheet evaluation during mass delete operations
        calculator = None
        parent = self.parent()
        while parent and not hasattr(parent, 'tabs'):
            parent = parent.parent()
        if parent and hasattr(parent, 'tabs'):
            calculator = parent
            if hasattr(calculator, '_mass_delete_in_progress') and calculator._mass_delete_in_progress:
                return
        
        # Pattern to detect cross-sheet references: S.SheetName.LN#
        cross_sheet_pattern = r'\bS\.[^.]+\.LN\d+\b'
        
        # Get current content
        lines = self.editor.toPlainText().split('\n')
        doc = self.results.document()
        
        # Store current cursor and scroll positions to restore them
        current_cursor = self.editor.textCursor()
        current_cursor_position = current_cursor.position()
        current_cursor_block = current_cursor.blockNumber()
        editor_scroll = self.editor.verticalScrollBar().value()
        results_scroll = self.results.verticalScrollBar().value()
        
        # Process each line and update only those with cross-sheet references
        out = [''] * len(lines)  # Initialize with empty strings to maintain line count
        
        # First, preserve all existing results
        for i in range(min(len(lines), doc.blockCount())):
            block = doc.findBlockByNumber(i)
            if block.isValid():
                out[i] = block.text()
        
        # Now evaluate only cross-sheet reference lines
        vals = {}
        cross_sheet_lines_updated = 0
        
        for i, line in enumerate(lines):
            line = line.strip()
            if not line:
                out[i] = ''
                continue
            
            # Check if this line has cross-sheet references
            if re.search(cross_sheet_pattern, line, re.IGNORECASE):
                try:
                    # Process cross-sheet references
                    processed_line = self.editor.process_ln_refs(line)
                    
                    # Evaluate the processed line
                    result = eval(processed_line, globals(), vals)
                    
                    # Store for later lines that might reference this one
                    vals[f'LN{i+1}'] = result
                    
                    # Format the result
                    formatted_result = self.format_number_for_display(result, i+1)
                    out[i] = formatted_result
                    cross_sheet_lines_updated += 1
                    
                except Exception as e:
                    out[i] = f"Error: {str(e)}"
            # For non-cross-sheet lines, preserve the LN values for reference
            elif line and not line.startswith('//'):
                try:
                    # Still need to evaluate to maintain LN values, but don't change output
                    result = eval(line, globals(), vals)
                    vals[f'LN{i+1}'] = result
                except:
                    pass  # Keep existing result
        
        # Update results display while maintaining line alignment
        cursor = QTextCursor(doc)
        cursor.movePosition(QTextCursor.Start)
        cursor.beginEditBlock()
        
        # Clear and rebuild the entire results to ensure proper line alignment
        cursor.select(QTextCursor.Document)
        cursor.removeSelectedText()
        
        # Insert all results line by line
        for i, result_text in enumerate(out):
            if i > 0:
                cursor.insertText('\n')
            cursor.insertText(result_text)
        
        cursor.endEditBlock()
        
        # Restore cursor position properly
        restored_cursor = QTextCursor(self.editor.document())
        restored_cursor.setPosition(current_cursor_position)
        self.editor.setTextCursor(restored_cursor)
        
        # Restore scroll positions using the sync mechanism
        self._syncing_scroll = True  # Temporarily disable sync to avoid double-setting
        self.editor.verticalScrollBar().setValue(editor_scroll)
        self.results.verticalScrollBar().setValue(results_scroll)
        self._syncing_scroll = False
        
        # Force highlighting update to ensure cursor line is properly highlighted
        self.editor.highlightCurrentLine()
        
        # Force a delayed sync to ensure perfect alignment
        QTimer.singleShot(10, lambda: self._force_sync_from_editor())
        
        # print(f"⚡ Cross-sheet selective evaluation: {cross_sheet_lines_updated} lines updated")  # Comment out for normal usage

    def _initialize_evaluation(self):
        """Initialize evaluation state and prepare data structures"""
        evaluation_context = {}
        
        # Store current cursor and scroll positions
        evaluation_context['cursor'] = self.editor.textCursor()
        evaluation_context['editor_scroll'] = self.editor.verticalScrollBar().value()
        evaluation_context['results_scroll'] = self.results.verticalScrollBar().value()
        
        # Check if this is a cursor-triggered evaluation
        evaluation_context['is_cursor_triggered'] = hasattr(self.editor, '_cursor_triggered_eval')
        if evaluation_context['is_cursor_triggered']:
            delattr(self.editor, '_cursor_triggered_eval')
        
        # Ensure line IDs are properly assigned
        self.editor.reassign_line_ids()
        
        # Initialize data structures
        evaluation_context['lines'] = self.editor.toPlainText().split("\n")
        evaluation_context['vals'] = [None] * len(evaluation_context['lines'])
        self.editor.ln_value_map = {}
        
        # Stage 2 Optimization: Clear LN reference cache when values change
        self.editor.clear_ln_reference_cache()
        
        id_map = {}
        evaluation_context['doc'] = self.editor.document()
        
        # Update separator lines
        self.editor.update_separator_lines()
        
        # Build id_map and initialize ln_value_map
        for i in range(evaluation_context['doc'].blockCount()):
            blk = evaluation_context['doc'].findBlockByNumber(i)
            d = blk.userData()
            if isinstance(d, LineData):
                id_map[d.id] = i
                # Initialize with None to ensure the ID exists in the map
                self.editor.ln_value_map[d.id] = None
        
        evaluation_context['id_map'] = id_map
        return evaluation_context

class Calculator(QWidget):
    def __init__(self):
        super().__init__()
        # Set window flags to ensure it appears on top initially
        self.setWindowFlags(self.windowFlags() | Qt.WindowStaysOnTopHint)
        
        # Set window icon
        icon_path = os.path.join(os.path.dirname(sys.argv[0]), "calcforge.ico")
        if os.path.exists(icon_path):
            self.setWindowIcon(QIcon(icon_path))
        
        # Tab switching optimization - Stage 1: Change tracking system
        self._sheet_changed_flags = {}  # Tab index -> bool (True if sheet content changed)
        self._last_active_sheet = None  # Track the previously active sheet index
        
        # Stage 3: Dependency Graph Optimization
        self._sheet_dependencies = {}  # Tab index -> set of tab indices that this sheet references
        self._sheet_dependents = {}   # Tab index -> set of tab indices that reference this sheet
        self._pending_updates = set() # Tab indices that need evaluation due to dependency cascade
        self._batch_update_timer = QTimer()
        self._batch_update_timer.setSingleShot(True)
        self._batch_update_timer.timeout.connect(self._process_batch_updates)
        
        # Undo system initialization
        self.undo_manager = UndoManager(max_undo_states=200)
        
        self.settings = QSettings('OpenAI','SmartCalc')
        if self.settings.contains('geometry'):
            self.restoreGeometry(self.settings.value('geometry'))
        self.splitter_state = self.settings.value('splitterState')
        self.setWindowTitle("CalcForge v4.0")
        self.setMinimumSize(800,400)
        
        # Remove stay on top flag after initial setup
        self.setWindowFlags(self.windowFlags())  # Keep the WindowStaysOnTopHint flag since checkbox is checked by default
        self.show()  # Need to show again after changing flags
        
        main=QVBoxLayout(self)
        top=QHBoxLayout()
        
        # Add buttons
        add_btn=QPushButton("+")
        add_btn.setFixedWidth(30)
        add_btn.clicked.connect(self.add_tab)
        
        help_btn=QPushButton("?")
        help_btn.setFixedWidth(30)
        help_btn.clicked.connect(self.show_help)
        
        # Add stay on top checkbox
        self.stay_on_top = QCheckBox("Stay on Top")
        self.stay_on_top.setChecked(True)  # Set to checked by default
        self.stay_on_top.stateChanged.connect(self.toggle_stay_on_top)
        # Add stylesheet to make checkbox more visible
        self.stay_on_top.setStyleSheet("""
            QCheckBox {
                color: #ffffff;
                padding: 2px;
                border: 1px solid #4477ff;
                border-radius: 4px;
                background-color: #3a3a3d;
            }
            QCheckBox::indicator {
                width: 13px;
                height: 13px;
                border: 1px solid #4477ff;
                border-radius: 2px;
                background-color: #2c2c2e;
            }
            QCheckBox::indicator:checked {
                background-color: #4477ff;
            }
        """)
        
        top.addWidget(add_btn)
        top.addWidget(help_btn)
        top.addWidget(self.stay_on_top)
        top.addStretch()
        main.addLayout(top)
        
        self.tabs=QTabWidget()
        self.tabs.setTabsClosable(True)
        self.tabs.setMovable(True)  # Make tabs reorderable
        self.tabs.tabCloseRequested.connect(self.close_tab)
        self.tabs.tabBarDoubleClicked.connect(self.rename_tab)
        self.tabs.currentChanged.connect(self.on_tab_changed)  # Connect tab change signal
        main.addWidget(self.tabs)
        
        # Load saved worksheets or create new one
        wf = Path(os.path.dirname(sys.argv[0]))/"worksheets.json"
        if wf.exists():
            try:
                data = json.loads(wf.read_text())
                self.tabs.clear()
                for idx, (name, content) in enumerate(data.items()):
                    ws = Worksheet(self)
                    self.tabs.addTab(ws, name)
                    ws.editor.setPlainText(content)
                    if self.splitter_state:
                        ws.splitter.restoreState(self.splitter_state)
                    # Tab switching optimization - Initialize change flag for loaded sheet
                    self._sheet_changed_flags[idx] = False
                # Position cursor at end of first sheet
                if self.tabs.count() > 0:
                    self.position_cursor_at_end(self.tabs.widget(0).editor)
                    
                # Stage 3: Build initial dependency graph for loaded worksheets
                self.build_dependency_graph()
                
                # Capture initial state for undo system
                self.undo_manager.capture_state(self)
            except:
                self.add_tab()  # Add default tab if loading fails
        else:
            self.add_tab()  # Add default tab if no saved worksheets
            
    def position_cursor_at_end(self, editor):
        """Position cursor at the end of the editor, adding a new line if needed"""
        # Get the document
        doc = editor.document()
        
        # Get the last block
        last_block = doc.lastBlock()
        last_text = last_block.text().strip()
        
        # Create cursor at the end
        cursor = editor.textCursor()
        cursor.movePosition(QTextCursor.End)
        
        # If last line is not empty, add a new line
        if last_text:
            cursor.insertText("\n")
            
        # Set the cursor
        editor.setTextCursor(cursor)
        editor.setFocus()

    def toggle_stay_on_top(self, state):
        flags = self.windowFlags()
        if state:
            flags |= Qt.WindowStaysOnTopHint
        else:
            flags &= ~Qt.WindowStaysOnTopHint
        self.setWindowFlags(flags)
        self.show()  # Need to show again after changing flags

    def keyPressEvent(self, event):
        """Handle keyboard shortcuts for the Calculator window"""
        # Check for modifiers and key
        shift = event.modifiers() & Qt.ShiftModifier
        ctrl = event.modifiers() & Qt.ControlModifier
        key = event.key()
        
        # Ctrl+Z: Undo
        if ctrl and key == Qt.Key_Z and not shift:
            if self.undo_manager.undo(self):
                # Undo successful
                pass
            event.accept()
            return
        
        # Ctrl+Y: Redo
        elif ctrl and key == Qt.Key_Y:
            if self.undo_manager.redo(self):
                # Redo successful
                pass
            event.accept()
            return
        
        # Shift+Ctrl+Left/Right for tab navigation
        elif shift and ctrl:
            if key == Qt.Key_Left:
                # Navigate to previous tab
                current_index = self.tabs.currentIndex()
                if current_index > 0:
                    self.tabs.setCurrentIndex(current_index - 1)
                else:
                    # Wrap to last tab
                    self.tabs.setCurrentIndex(self.tabs.count() - 1)
                event.accept()
                return
            elif key == Qt.Key_Right:
                # Navigate to next tab
                current_index = self.tabs.currentIndex()
                if current_index < self.tabs.count() - 1:
                    self.tabs.setCurrentIndex(current_index + 1)
                else:
                    # Wrap to first tab
                    self.tabs.setCurrentIndex(0)
                event.accept()
                return
        
        # Call parent implementation for other keys
        super().keyPressEvent(event)

    def add_tab(self):
        ws=Worksheet()
        idx=self.tabs.addTab(ws,f"Sheet {self.tabs.count()+1}")
        self.tabs.setCurrentIndex(idx)
        if self.splitter_state:
            ws.splitter.restoreState(self.splitter_state)
        # Position cursor at end for new tabs
        self.position_cursor_at_end(ws.editor)
        
        # Tab switching optimization - Initialize change flag for new sheet
        self._sheet_changed_flags[idx] = False

        # Stage 3: Initialize dependency tracking for new sheet
        self._sheet_dependencies[idx] = set()
        # Rebuild dependency graph to account for new sheet
        self.build_dependency_graph()

        # Invalidate cross-sheet caches in all editors
        self.invalidate_all_cross_sheet_caches()

        # Initialize mass delete flag for cross-tab evaluation control
        if not hasattr(self, '_mass_delete_in_progress'):
            self._mass_delete_in_progress = False

        # Capture state for undo system
        self.undo_manager.capture_state(self)

    def close_tab(self,idx):
        if self.tabs.count()>1: 
            # Get the sheet name for the confirmation dialog
            sheet_name = self.tabs.tabText(idx)
            
            # Show confirmation dialog
            reply = QMessageBox.question(
                self, 
                "Confirm Sheet Deletion", 
                f"Are you sure you want to delete sheet '{sheet_name}'?\n\n(This action can be undone with Ctrl+Z)",
                QMessageBox.Yes | QMessageBox.No,
                QMessageBox.No  # Default to No for safety
            )
            
            # Only proceed with deletion if user confirms
            if reply == QMessageBox.Yes:
                self.tabs.removeTab(idx)
                
                # Tab switching optimization - Clean up change flags for removed tab
                if idx in self._sheet_changed_flags:
                    del self._sheet_changed_flags[idx]
                
                # Stage 3: Clean up dependency tracking for removed tab
                if idx in self._sheet_dependencies:
                    del self._sheet_dependencies[idx]
                if idx in self._sheet_dependents:
                    del self._sheet_dependents[idx]
                
                # Adjust change flags for tabs after the removed one (shift indices down)
                updated_flags = {}
                for tab_idx, changed in self._sheet_changed_flags.items():
                    if tab_idx > idx:
                        updated_flags[tab_idx - 1] = changed
                    else:
                        updated_flags[tab_idx] = changed
                self._sheet_changed_flags = updated_flags
                
                # Adjust dependency tracking for shifted indices
                updated_dependencies = {}
                updated_dependents = {}
                
                for tab_idx, deps in self._sheet_dependencies.items():
                    new_idx = tab_idx - 1 if tab_idx > idx else tab_idx
                    new_deps = set()
                    for dep_idx in deps:
                        if dep_idx != idx:  # Skip the removed tab
                            new_dep_idx = dep_idx - 1 if dep_idx > idx else dep_idx
                            new_deps.add(new_dep_idx)
                    updated_dependencies[new_idx] = new_deps
                
                for tab_idx, deps in self._sheet_dependents.items():
                    new_idx = tab_idx - 1 if tab_idx > idx else tab_idx
                    new_deps = set()
                    for dep_idx in deps:
                        if dep_idx != idx:  # Skip the removed tab
                            new_dep_idx = dep_idx - 1 if dep_idx > idx else dep_idx
                            new_deps.add(new_dep_idx)
                    updated_dependents[new_idx] = new_deps
                
                self._sheet_dependencies = updated_dependencies
                self._sheet_dependents = updated_dependents
                
                # Update last active sheet index if needed
                if self._last_active_sheet is not None:
                    if self._last_active_sheet == idx:
                        self._last_active_sheet = None
                    elif self._last_active_sheet > idx:
                        self._last_active_sheet -= 1
                        
                # Invalidate cross-sheet caches in all editors
                self.invalidate_all_cross_sheet_caches()

    def rename_tab(self,idx):
        if idx>=0:
            text,ok=QInputDialog.getText(self,"Rename Sheet","New name:")
            if ok and text: 
                self.tabs.setTabText(idx,text)
                
                # Stage 3: Rebuild dependency graph since sheet names affect cross-sheet references
                self.build_dependency_graph()
                
                # Invalidate cross-sheet caches in all editors
                self.invalidate_all_cross_sheet_caches()

    def closeEvent(self,event):
        self.settings.setValue('geometry',self.saveGeometry())
        ws=self.tabs.currentWidget()
        if hasattr(ws,'splitter'): self.settings.setValue('splitterState',ws.splitter.saveState())
        wf=Path(os.path.dirname(sys.argv[0]))/"worksheets.json"
        data={self.tabs.tabText(i):self.tabs.widget(i).editor.toPlainText() for i in range(self.tabs.count())}
        wf.write_text(json.dumps(data,indent=2))
        super().closeEvent(event)

    def show_help(self):
        # Create a custom dialog with 16:9 aspect ratio
        
        dialog = QDialog(self)
        dialog.setWindowTitle("CalcForge v4.0 Help")
        dialog.setWindowFlags(dialog.windowFlags() | Qt.WindowMaximizeButtonHint)
        
        # Set the icon for the dialog
        icon_path = os.path.join(os.path.dirname(sys.argv[0]), "calcforge.ico")
        if os.path.exists(icon_path):
            dialog.setWindowIcon(QIcon(icon_path))
        
        # Set 16:9 aspect ratio - using a reasonable size for readability
        width = 1200
        height = int(width * 9 / 16)  # 675
        dialog.resize(width, height)
        
        # Center the dialog on the parent window
        if self.geometry().isValid():
            parent_center = self.geometry().center()
            dialog.move(parent_center.x() - width // 2, parent_center.y() - height // 2)
        
        layout = QVBoxLayout(dialog)
        
        # Create text widget with dark mode styling
        text_widget = QTextEdit()
        text_widget.setReadOnly(True)
        text_widget.setStyleSheet("""
            QTextEdit {
                background-color: #1e1e1e;
                color: #e0e0e0;
                font-family: 'Segoe UI', Arial, sans-serif;
                font-size: 11pt;
                border: none;
                padding: 10px;
            }
        """)
        
        # Get the icon path for embedding in HTML
        icon_path = os.path.join(os.path.dirname(sys.argv[0]), "calcforge.ico")
        icon_html = ""
        if os.path.exists(icon_path):
            # Convert to file URL for HTML
            icon_url = f"file:///{icon_path.replace(os.sep, '/')}"
            icon_html = f"<img src='{icon_url}' width='32' height='32' style='vertical-align: middle; margin-right: 10px;'>"
        
        # Dark mode color scheme
        text = (
            "<div style='max-width: 100%; margin: 0 auto; background-color: #1e1e1e; color: #e0e0e0;'>"
            f"<h1 style='text-align: center; color: #4da6ff; margin-bottom: 20px;'>{icon_html}CalcForge v4.0 - Complete Reference Guide</h1>"
            
            "<table width='100%' cellpadding='12' cellspacing='0' style='border-collapse: collapse;'>"
            
            # First Row - Core Features (Dark blue theme)
            "<tr style='background-color: #2a2a3e;'>"
            "<td width='33%' valign='top' style='border-right: 1px solid #444; padding-right: 15px;'>"
            "<h3 style='color: #6fcf97; margin-top: 0;'>🧮 Basic Operations</h3>"
            "• Standard arithmetic: <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>+, -, *, /, ^</code><br>"
            "• Parentheses grouping: <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>(2 + 3) * 4</code><br>"
            "• Number formatting with commas<br>"
            "• Leading zero handling: <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>0123 → 123</code><br><br>"
            
            "<h3 style='color: #6fcf97;'>📊 Line References</h3>"
            "• Use <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>LN1, LN2</code>, etc. to reference lines<br>"
            "• Example: <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>LN1 + LN2</code><br>"
            "• Auto-capitalization: <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>ln1 → LN1</code><br>"
            "• Color-coded highlighting<br><br>"
            
            "<h3 style='color: #6fcf97;'>🔗 Cross-Sheet References</h3>"
            "• Format: <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>S.SheetName.LN2</code><br>"
            "• References line 2 from another sheet<br>"
            "• Sheet names are case-insensitive<br>"
            "• Auto-capitalization: <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>s.sheet.ln2 → S.Sheet.LN2</code><br>"
            "• Mix with regular refs: <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>LN1 + S.Data.LN3</code><br>"
            "• Cross-sheet highlighting<br>"
            "• Optimized dependency tracking<br>"
            "• Smart selective cache invalidation<br>"
            "</td>"
            
            "<td width='33%' valign='top' style='border-right: 1px solid #444; padding: 0 15px;'>"
            "<h3 style='color: #6fcf97; margin-top: 0;'>🔢 Mathematical Functions</h3>"
            "• <strong style='color: #ff9999;'>Trigonometric:</strong> <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>sin(), cos(), tan(), asin(), acos(), atan()</code><br>"
            "• <strong style='color: #ff9999;'>Hyperbolic:</strong> <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>sinh(), cosh(), tanh(), asinh(), acosh(), atanh()</code><br>"
            "• <strong style='color: #ff9999;'>Angle conversion:</strong> <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>degrees(), radians()</code><br>"
            "• <strong style='color: #ff9999;'>Power & Log:</strong> <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>sqrt(), pow(x,y), exp(), log(), log10(), log2()</code><br>"
            "• <strong style='color: #ff9999;'>Rounding:</strong> <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>ceil(), floor(), abs()</code><br>"
            "• <strong style='color: #ff9999;'>Other:</strong> <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>factorial(), gcd(), lcm()</code><br>"
            "• <strong style='color: #ff9999;'>Constants:</strong> <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>pi, e</code><br><br>"
            
            "<h3 style='color: #6fcf97;'>🔧 Number Formatting</h3>"
            "• <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>truncate(value, decimals)</code> or <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>TR(value, decimals)</code><br>"
            "• Works with all result types<br>"
            "• Example: <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>truncate(pi, 3) → 3.142</code><br>"
            "• Large numbers display with commas up to trillions<br>"
            "• Scientific notation only for values > 1e12<br><br>"
            
            "<h3 style='color: #6fcf97;'>🔄 Unit Conversions</h3>"
            "• Format: <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>value unit to target_unit</code><br>"
            "• <strong style='color: #ff9999;'>Length:</strong> <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>1 mile to km, 5 feet to meters</code><br>"
            "• <strong style='color: #ff9999;'>Weight:</strong> <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>10 pounds to kg, 2 tons to lbs</code><br>"
            "• <strong style='color: #ff9999;'>Volume:</strong> <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>5 gallons to liters, 2 cups to mL</code><br>"
            "• Abbreviations supported: lb, kg, ft, m, etc.<br>"
            "• Works with truncate(): <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>truncate(1 mile to km, 3)</code><br><br>"
            
            "<h3 style='color: #6fcf97;'>💱 Currency Conversions</h3>"
            "• Format: <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>value currency to target_currency</code><br>"
            "• <strong style='color: #ff9999;'>Examples:</strong> <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>20.40 dollars to euros, 100 yen to usd</code><br>"
            "• <strong style='color: #ff9999;'>Major currencies:</strong> USD, EUR, GBP, JPY, CAD, AUD, CHF<br>"
            "• <strong style='color: #ff9999;'>Also supports:</strong> CNY, INR, KRW, MXN, BRL, RUB, SEK, NOK<br>"
            "• Full names work: <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>50 pounds to canadian dollars</code><br>"
            "• Real-time exchange rates (when online)<br>"
            "• Works with truncate(): <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>truncate(100 usd to eur, 2)</code><br>"
            "</td>"
            
            "<td width='33%' valign='top' style='padding-left: 15px;'>"
            "<h3 style='color: #6fcf97;'>📈 Statistical Functions</h3>"
            "• <strong style='color: #ff9999;'>Basic Statistics:</strong><br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>sum(above), sum(below), sum(start range - end range), sum(line1,line2,line3)</code><br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>sum(cg-above), sum(cg-below)</code><br><br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>mean(), min(), max(), median(), mode()</code><br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>range(), count(), variance(), stdev()</code><br><br>"
            
            "• <strong style='color: #ff9999;'>Range Options:</strong><br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>above</code> - all lines above current<br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>below</code> - all lines below current<br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>start range - end range</code> - range of lines (e.g., 1-5)<br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>line1,line2,line3</code> - specific lines (e.g., 1,3,5)<br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>cg-above</code> - to nearest comment above<br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>cg-below</code> - to nearest comment below<br><br>"
            
            "• <strong style='color: #ff9999;'>Advanced Functions:</strong><br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>geomean(), harmmean(), product(), sumsq()</code><br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>perc5(), perc95()</code><br><br>"
            
            "• Empty parentheses use all lines above<br>"
            "• Visual separators for stat blocks<br><br>"
            
            "<h3 style='color: #6fcf97;'>🎬 Timecode Functions</h3>"
            "• <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>TC(fps, timecode)</code> or <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>TC(fps, frames)</code><br>"
            "• <strong style='color: #ff9999;'>Frame rates:</strong> 23.976, 24, 25, 29.97 DF, 30, 50, 59.94 DF, 60<br>"
            "• <strong style='color: #ff9999;'>Format:</strong> HH:MM:SS:FF (: or . separators)<br>"
            "• <strong style='color: #ff9999;'>Arithmetic:</strong> <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>TC(29.97, \"01:00:00:00\" + \"00:30:00:00\")</code><br>"
            "• <strong style='color: #ff9999;'>Conversion:</strong> <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>TC(24, 1440) → timecode</code><br><br>"
            
            "<h3 style='color: #6fcf97;'>📐 Aspect Ratio Calculator</h3>"
            "• <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>AR(original_dimensions, target_dimensions)</code><br>"
            "• <strong style='color: #ff9999;'>Format:</strong> <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>AR(1920x1080, ?x2000)</code> - solve for width<br>"
            "• <strong style='color: #ff9999;'>Or:</strong> <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>AR(1920x1080, 1280x?)</code> - solve for height<br>"
            "• <strong style='color: #ff9999;'>Common ratios:</strong> 16:9 (1920x1080), 4:3 (1024x768), 21:9 (2560x1080)<br>"
            "• Case insensitive: <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>ar()</code> or <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>AR()</code><br>"
            "</td>"
            "</tr>"
            
            # Second Row - Date Operations (Dark purple theme)
            "<tr style='background-color: #3e2a3e;'>"
            "<td colspan='3' valign='top' style='padding-top: 15px;'>"
            "<h3 style='color: #bb86fc; margin-top: 0;'>📅 Date Operations</h3>"
            "<table width='100%'><tr>"
            "<td width='50%'>"
            "<strong style='color: #ff9999;'>Date Formats (D, D., d, d. prefixes):</strong><br>"
            "• <strong style='color: #ff9999;'>Numeric:</strong> <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>D.05.09.2030, D05.09.2030, D.05092030, D.592030</code><br>"
            "• <strong style='color: #ff9999;'>Month names:</strong> <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>D.March 5, 1976, D.Mar 5, 1976, DMarch 5, 1976</code><br><br>"
            
            "<strong style='color: #ff9999;'>Regular Date Arithmetic:</strong><br>"
            "• Add days: <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>D.03.05.1976 + 100</code><br>"
            "• Subtract days: <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>D.03.05.1976 - 50</code><br>"
            "• Date range: <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>D.03.05.1976 - D.04.15.1976</code><br>"
            "• Single date: <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>D.12.25.2024</code><br>"
            "</td>"
            "<td width='50%'>"
            "<strong style='color: #ff9999;'>Business Day Calculations (skips weekends):</strong><br>"
            "• Add business days: <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>D.03.05.1976 W+ 100</code><br>"
            "• Subtract business days: <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>D.03.05.1976 W- 50</code><br>"
            "• Business days between dates: <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>D.03.05.1976 W- D.04.15.1976</code><br>"
            "</td>"
            "</tr></table>"
            "</td>"
            "</tr>"
            
            # Third Row - Interface & Features (Dark green theme)
            "<tr style='background-color: #2a3e2a;'>"
            "<td width='33%' valign='top' style='border-right: 1px solid #444; padding-right: 15px;'>"
            "<h3 style='color: #81c784; margin-top: 0;'>💻 Interface Features</h3>"
            "• Multi-sheet support with tabs<br>"
            "• Drag to reorder sheets<br>"
            "• Double-click to rename sheets<br>"
            "• Real-time syntax highlighting<br>"
            "• Context-aware auto-completion for functions<br>"
            "• Live result updates<br>"
            "• Perfect scrollbar synchronization<br>"
            "• Stay on top option<br><br>"
            
            "<h3 style='color: #81c784;'>📝 Comments & Organization</h3>"
            "• Comment lines: Start with <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>:::</code><br>"
            "• Comments show as 'C' in line numbers<br>"
            "• Auto-save worksheets<br>"
            "• Persistent settings<br>"
            "</td>"
            
            "<td width='33%' valign='top' style='border-right: 1px solid #444; padding: 0 15px;'>"
            "<h3 style='color: #81c784; margin-top: 0;'>⌨️ Keyboard Shortcuts</h3>"
            "• <strong style='color: #ff9999;'>Navigation:</strong><br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>Ctrl+Left/Right</code>: Jump between numbers<br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>Ctrl+Up</code>: Expand selection with parentheses<br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>Ctrl+Down</code>: Select entire line<br><br>"
            
            "• <strong style='color: #ff9999;'>Tab Navigation:</strong><br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>Shift+Ctrl+Left</code>: Previous sheet<br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>Shift+Ctrl+Right</code>: Next sheet<br><br>"
            
            "• <strong style='color: #ff9999;'>Copying:</strong><br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>Ctrl+C</code>: Copy result from current line<br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>Alt+C</code>: Copy line content<br><br>"
            
            "• <strong style='color: #ff9999;'>Font scaling:</strong><br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>Ctrl+Mouse wheel</code>: Zoom in/out<br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>Ctrl+Middle click</code>: Reset font size<br>"
            "</td>"
            
            "<td width='33%' valign='top' style='padding-left: 15px;'>"
            "<h3 style='color: #81c784; margin-top: 0;'>🎨 Visual Features</h3>"
            "• Color-coded LN references<br>"
            "• Live highlighting of referenced lines<br>"
            "• Cross-sheet reference highlighting<br>"
            "• Expression tooltips on hover<br>"
            "• Operator result previews<br>"
            "• Visual separators for stat functions<br>"
            "• Line numbers with comment indicators<br><br>"
            
            "<h3 style='color: #81c784;'>💡 Pro Tips</h3>"
            "• Hover over operators to see sub-results<br>"
            "• Use empty stat functions for all lines above<br>"
            "• Mix different calculation types freely<br>"
            "• Reference across sheets for complex workflows<br>"
            "</td>"
            "</tr>"
            
            "</table>"
            "</div>"
        )
        
        text_widget.setHtml(text)
        layout.addWidget(text_widget)
        
        # Add close button with dark styling
        button_layout = QHBoxLayout()
        button_layout.addStretch()
        close_button = QPushButton("Close")
        close_button.setFixedSize(100, 30)
        close_button.setStyleSheet("""
            QPushButton {
                background-color: #4da6ff;
                color: white;
                border: none;
                border-radius: 5px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #3d8bdb;
            }
            QPushButton:pressed {
                background-color: #2c5aa0;
            }
        """)
        close_button.clicked.connect(dialog.accept)
        close_button.setDefault(True)
        button_layout.addWidget(close_button)
        layout.addLayout(button_layout)
        
        dialog.exec()
  
    def force_focus(self):
        """Force the window and current editor to get focus - useful for keyboard shortcuts"""
        # Force window to front and activate
        self.raise_()
        self.activateWindow()
        self.setFocus()
        
        # Set focus to the current editor
        current_widget = self.tabs.currentWidget()
        if current_widget and hasattr(current_widget, 'editor'):
            current_widget.editor.setFocus()
            current_widget.editor.activateWindow()

    def on_tab_changed(self, index):
        """Smart tab switching with Stage 3 dependency graph optimization"""
        if index < 0:  # Invalid tab index
            return

        current_sheet = self.tabs.widget(index)
        if not current_sheet or not hasattr(current_sheet, 'evaluate'):
            return

        # Clear mass delete flag when switching away from the tab that had the mass delete
        if hasattr(self, '_mass_delete_in_progress') and self._mass_delete_in_progress:
            mass_delete_tab_index = getattr(self, '_mass_delete_tab_index', -1)
            if index != mass_delete_tab_index:
                print(f"DEBUG: Clearing mass delete flag due to tab switch from {mass_delete_tab_index} to {index}")
                self._mass_delete_in_progress = False
                if hasattr(self, '_mass_delete_tab_index'):
                    delattr(self, '_mass_delete_tab_index')
        
        # Debug output - ENABLE THIS TO DIAGNOSE TAB SWITCHING PERFORMANCE
        DEBUG_TAB_SWITCHING = False  # Set to True to enable debug output
        if DEBUG_TAB_SWITCHING:
            print(f"\n=== TAB SWITCH DEBUG ===")
            print(f"Switching to tab {index} ({self.tabs.tabText(index)}), last active: {self._last_active_sheet}")
            print(f"Change flags: {self._sheet_changed_flags}")
            print(f"Current sheet has cross-refs: {getattr(current_sheet, 'has_cross_sheet_refs', 'NOT_SET')}")
            if hasattr(self, '_sheet_dependencies'):
                print(f"Dependencies for tab {index}: {self._sheet_dependencies.get(index, set())}")
                print(f"Dependents of tab {index}: {self._sheet_dependents.get(index, set())}")
        
        # Check if evaluation is needed
        should_evaluate = False
        reason = ""
        
        # Stage 3: Check if this sheet has dependencies on recently changed sheets
        needs_dependency_update = False
        if (hasattr(self, '_sheet_dependencies') and 
            index in self._sheet_dependencies):
            # Check if any of the sheets this one depends on have changed
            dependencies = self._sheet_dependencies[index]
            for dep_idx in dependencies:
                if self._sheet_changed_flags.get(dep_idx, False):
                    needs_dependency_update = True
                    break
        
        # Case 1: This sheet depends on recently changed sheets (Stage 3 optimization)
        if needs_dependency_update:
            should_evaluate = True
            reason = f"Sheet {index} depends on changed sheets"
            
        # Case 2: Previous sheet was modified and current sheet has cross-sheet references (Stage 1/2)
        elif (self._last_active_sheet is not None and 
            self._sheet_changed_flags.get(self._last_active_sheet, False) and
            hasattr(current_sheet, 'has_cross_sheet_refs') and
            current_sheet.has_cross_sheet_refs):
            should_evaluate = True
            reason = f"Previous sheet {self._last_active_sheet} changed + current has cross-refs"
            
        # Case 3: Current sheet was modified (always evaluate)
        elif self._sheet_changed_flags.get(index, False):
            should_evaluate = True
            reason = f"Current sheet {index} was modified"
            
        # Case 4: First time switch to this tab or no change tracking data yet
        elif index not in self._sheet_changed_flags:
            should_evaluate = True
            reason = f"First time switch to sheet {index}"
            # Initialize change flag for new sheets
            self._sheet_changed_flags[index] = False

        if DEBUG_TAB_SWITCHING:
            print(f"Should evaluate: {should_evaluate}")
            if should_evaluate:
                print(f"Reason: {reason}")
            else:
                print("Reason: No changes detected - SKIPPING EVALUATION ✅ [STAGE 1 OPTIMIZATION]")
            print("========================\n")

        if should_evaluate:
            # Invalidate cross-sheet caches to ensure fresh lookups
            self.invalidate_all_cross_sheet_caches()
            
            # Stage 2 Fix: Reset highlighting and navigation state when switching tabs
            if hasattr(current_sheet, 'editor'):
                # Reset rapid navigation state that might interfere with proper highlighting
                current_sheet.editor._is_rapid_navigation = False
                current_sheet.editor._nav_move_count = 0
                current_sheet.editor._last_nav_time = 0
                # Stop any pending rapid navigation timer
                if hasattr(current_sheet.editor, '_rapid_nav_timer'):
                    current_sheet.editor._rapid_nav_timer.stop()
            
            # Remove premature change flag clearing - do it after evaluation instead
            # self._sheet_changed_flags[index] = False  # MOVED TO AFTER EACH EVALUATION TYPE
            
            # Stage 3: Use dependency-aware evaluation
            if needs_dependency_update:
                # Only evaluate cross-sheet lines for dependency updates
                if DEBUG_TAB_SWITCHING:
                    print("🚀 [STAGE 3] Dependency-aware selective evaluation")
                current_sheet.evaluate_cross_sheet_lines_only()
                # Clear change flags for dependencies that were just processed
                if hasattr(self, '_sheet_dependencies'):
                    dependencies = self._sheet_dependencies[index]
                    for dep_idx in dependencies:
                        self._sheet_changed_flags[dep_idx] = False
            # Stage 2 optimization: Use selective evaluation when only cross-sheet updates are needed
            elif (reason.startswith("Previous sheet") and 
                "changed + current has cross-refs" in reason and
                hasattr(current_sheet, 'evaluate_cross_sheet_lines_only')):
                # Only evaluate cross-sheet reference lines for better performance
                if DEBUG_TAB_SWITCHING:
                    print("⚡ [STAGE 2] Cross-sheet selective evaluation")
                current_sheet.evaluate_cross_sheet_lines_only()
                # Clear the change flag for the previous sheet since we processed its changes
                if self._last_active_sheet is not None:
                    self._sheet_changed_flags[self._last_active_sheet] = False
            else:
                # Full evaluation for other cases (current sheet modified, first-time switch)
                if DEBUG_TAB_SWITCHING:
                    print("📊 [FULL EVAL] Complete sheet evaluation")
                current_sheet.evaluate()
                # Clear the change flag for the current sheet after full evaluation
                self._sheet_changed_flags[index] = False
        
        # Stage 2 Fix: Force proper highlighting after tab switch and evaluation
        if should_evaluate and hasattr(current_sheet, 'editor'):
            # Force highlighting update with a slight delay to ensure evaluation is complete
            QTimer.singleShot(50, lambda: current_sheet.editor.highlightCurrentLine())
        
        # Clear change flag for previous sheet if it was just evaluated due to dependencies
        if self._last_active_sheet is not None and needs_dependency_update:
            # Don't clear the flag if the sheet was actually modified by user
            pass
        
        # Update last active sheet
        self._last_active_sheet = index

    def invalidate_all_cross_sheet_caches(self):
        """Invalidate cross-sheet caches in all editor instances"""
        for i in range(self.tabs.count()):
            sheet = self.tabs.widget(i)
            if hasattr(sheet, 'editor'):
                # Clear original cross-sheet cache
                if hasattr(sheet.editor, '_cross_sheet_cache'):
                    sheet.editor._cross_sheet_cache.clear()
                # Stage 2 Fix: Also clear LN reference cache when cross-sheet values change
                if hasattr(sheet.editor, '_ln_reference_cache'):
                    sheet.editor._ln_reference_cache.clear()

    def build_dependency_graph(self):
        """Stage 3: Build complete dependency graph for all sheets"""
        # Clear existing dependencies
        self._sheet_dependencies.clear()
        self._sheet_dependents.clear()
        
        # Pattern to detect cross-sheet references: S.SheetName.LN#
        cross_sheet_pattern = r'\bS\.([^.]+)\.LN\d+\b'
        
        for sheet_idx in range(self.tabs.count()):
            sheet = self.tabs.widget(sheet_idx)
            if not sheet or not hasattr(sheet, 'editor'):
                continue
                
            # Initialize dependencies for this sheet
            self._sheet_dependencies[sheet_idx] = set()
            
            # Get sheet content and find cross-sheet references
            content = sheet.editor.toPlainText()
            matches = re.finditer(cross_sheet_pattern, content, re.IGNORECASE)
            
            for match in matches:
                referenced_sheet_name = match.group(1).lower()
                
                # Find the tab index for this sheet name
                for target_idx in range(self.tabs.count()):
                    target_sheet_name = self.tabs.tabText(target_idx).lower()
                    if target_sheet_name == referenced_sheet_name:
                        # This sheet (sheet_idx) depends on target_idx
                        self._sheet_dependencies[sheet_idx].add(target_idx)
                        
                        # Add reverse dependency: target_idx has sheet_idx as a dependent
                        if target_idx not in self._sheet_dependents:
                            self._sheet_dependents[target_idx] = set()
                        self._sheet_dependents[target_idx].add(sheet_idx)
                        break
        
        # Print dependency summary (only in debug mode)
        DEBUG_TAB_SWITCHING = False  # This should match the debug flag
        if DEBUG_TAB_SWITCHING:
            print("📊 Dependency Graph Summary:")
            any_deps = False
            for sheet_idx in range(self.tabs.count()):
                sheet_name = self.tabs.tabText(sheet_idx)
                deps = self._sheet_dependencies.get(sheet_idx, set())
                dependents = self._sheet_dependents.get(sheet_idx, set())
                
                if deps or dependents:
                    any_deps = True
                    print(f"  {sheet_name} (#{sheet_idx}):")
                    if deps:
                        dep_names = [self.tabs.tabText(i) for i in deps]
                        print(f"    Depends on: {dep_names}")
                    if dependents:
                        dep_names = [self.tabs.tabText(i) for i in dependents]
                        print(f"    Referenced by: {dep_names}")
            
            if not any_deps:
                print("  No cross-sheet dependencies found")
            print("✅ Dependency graph complete\n")
    
    def get_dependent_sheets(self, changed_sheet_idx):
        """Stage 3: Get all sheets that depend on the changed sheet"""
        return self._sheet_dependents.get(changed_sheet_idx, set())
    
    def schedule_dependency_update(self, changed_sheet_idx):
        """Stage 3: Schedule updates for all sheets dependent on the changed sheet"""
        dependent_sheets = self.get_dependent_sheets(changed_sheet_idx)
        
        if dependent_sheets:
            # print(f"⏱️  [STAGE 3] Scheduling dependency updates for sheets: {dependent_sheets}")  # Comment out for normal usage
            # Add dependent sheets to pending updates
            self._pending_updates.update(dependent_sheets)
            
            # Start/restart the batch timer (50ms delay to batch multiple changes)
            self._batch_update_timer.start(50)
    
    def _process_batch_updates(self):
        """Stage 3: Process all pending dependency updates in a batch"""
        if not self._pending_updates:
            return

        # Skip batch updates during mass delete operations
        if hasattr(self, '_mass_delete_in_progress') and self._mass_delete_in_progress:
            return

        # print(f"🔄 [STAGE 3] Processing batch updates for sheets: {self._pending_updates}")  # Comment out for normal usage
        
        # Process each pending update
        for sheet_idx in self._pending_updates:
            if sheet_idx < self.tabs.count():
                sheet = self.tabs.widget(sheet_idx)
                if sheet and hasattr(sheet, 'evaluate_cross_sheet_lines_only'):
                    # Use selective evaluation for dependency updates
                    sheet.evaluate_cross_sheet_lines_only()
        
        # print("✅ [STAGE 3] Batch updates complete")  # Comment out for normal usage
        # Clear pending updates
        self._pending_updates.clear()

class UndoManager:
    """Manages undo/redo functionality for the calculator with a FIFO buffer"""
    
    def __init__(self, max_undo_states=200):
        self.max_undo_states = max_undo_states
        self.undo_stack = []
        self.redo_stack = []
        self._last_saved_state = None
    
    def capture_state(self, calculator):
        """Capture the current state of all sheets"""
        # Don't capture identical consecutive states
        current_state = self._create_state_snapshot(calculator)
        if current_state == self._last_saved_state:
            return
        
        # Add to undo stack with FIFO eviction
        self.undo_stack.append(current_state)
        if len(self.undo_stack) > self.max_undo_states:
            self.undo_stack.pop(0)  # Remove oldest state
        
        # Clear redo stack when new state is captured
        self.redo_stack.clear()
        self._last_saved_state = current_state
    
    def _create_state_snapshot(self, calculator):
        """Create a snapshot of the current calculator state"""
        state = {
            'sheets': [],
            'active_tab_index': calculator.tabs.currentIndex(),
            'timestamp': time.time()
        }
        
        # Capture all sheet data
        for i in range(calculator.tabs.count()):
            sheet = calculator.tabs.widget(i)
            cursor_position = sheet.editor.textCursor().position()
            sheet_data = {
                'name': calculator.tabs.tabText(i),
                'content': sheet.editor.toPlainText(),
                'cursor_position': cursor_position
            }
            state['sheets'].append(sheet_data)
        
        return state
    
    def undo(self, calculator):
        """Perform undo operation"""
        if not self.can_undo():
            return False
        
        # Save current state to redo stack before undoing
        current_state = self._create_state_snapshot(calculator)
        self.redo_stack.append(current_state)
        
        # Get the state to restore
        state_to_restore = self.undo_stack.pop()
        self._restore_state(calculator, state_to_restore)
        
        # Update last saved state
        self._last_saved_state = state_to_restore
        return True
    
    def redo(self, calculator):
        """Perform redo operation"""
        if not self.can_redo():
            return False
        
        # Save current state to undo stack before redoing
        current_state = self._create_state_snapshot(calculator)
        self.undo_stack.append(current_state)
        
        # Get the state to restore
        state_to_restore = self.redo_stack.pop()
        self._restore_state(calculator, state_to_restore)
        
        # Update last saved state
        self._last_saved_state = state_to_restore
        return True
    
    def _restore_state(self, calculator, state):
        """Restore calculator to a previous state"""
        # Temporarily disconnect text change signals to avoid capturing undo states during restoration
        self._disconnect_text_signals(calculator)
        
        try:
            # Restore sheets (add missing ones, remove extras, update existing)
            target_sheet_count = len(state['sheets'])
            current_sheet_count = calculator.tabs.count()
            
            # Add missing sheets
            while calculator.tabs.count() < target_sheet_count:
                calculator.add_tab()
            
            # Remove extra sheets (from the end)
            while calculator.tabs.count() > target_sheet_count:
                calculator.tabs.removeTab(calculator.tabs.count() - 1)
            
            # Restore each sheet's content and name
            for i, sheet_data in enumerate(state['sheets']):
                sheet = calculator.tabs.widget(i)
                
                # Restore sheet name
                calculator.tabs.setTabText(i, sheet_data['name'])
                
                # Restore content
                sheet.editor.setPlainText(sheet_data['content'])
                
                # Restore cursor position
                cursor = sheet.editor.textCursor()
                cursor.setPosition(min(sheet_data['cursor_position'], len(sheet_data['content'])))
                sheet.editor.setTextCursor(cursor)
                
                # Re-evaluate the sheet
                sheet.evaluate()
            
            # Restore active tab
            if 0 <= state['active_tab_index'] < calculator.tabs.count():
                calculator.tabs.setCurrentIndex(state['active_tab_index'])
            
            # Rebuild dependencies and caches
            calculator.build_dependency_graph()
            calculator.invalidate_all_cross_sheet_caches()
            
        finally:
            # Reconnect text change signals
            self._reconnect_text_signals(calculator)
    
    def _disconnect_text_signals(self, calculator):
        """Temporarily disconnect text change signals during restoration"""
        for i in range(calculator.tabs.count()):
            sheet = calculator.tabs.widget(i)
            try:
                sheet.editor.textChanged.disconnect()
            except:
                pass  # Signal might not be connected
    
    def _reconnect_text_signals(self, calculator):
        """Reconnect text change signals after restoration"""
        for i in range(calculator.tabs.count()):
            sheet = calculator.tabs.widget(i)
            sheet.editor.textChanged.connect(sheet.on_text_potentially_changed)
    
    def can_undo(self):
        """Check if undo is available"""
        return len(self.undo_stack) > 0
    
    def can_redo(self):
        """Check if redo is available"""
        return len(self.redo_stack) > 0
    
    def clear(self):
        """Clear all undo/redo history"""
        self.undo_stack.clear()
        self.redo_stack.clear()
        self._last_saved_state = None
    
    def get_memory_usage_estimate(self):
        """Get estimated memory usage of undo system in bytes"""
        total_size = 0
        for state in self.undo_stack + self.redo_stack:
            for sheet in state['sheets']:
                total_size += len(sheet['content']) * 2  # Rough estimate for Unicode
                total_size += len(sheet['name']) * 2
            total_size += 100  # Overhead for other state data
        return total_size

def verify_icon_file(icon_path):
    """Verify that the icon file contains all required sizes."""
    try:
        from PIL import Image
        
        # Open the ICO file
        with Image.open(icon_path) as img:
            if not img.is_animated:
                return False
        
            # Check each size in the icon
            sizes_found = []
            for frame in range(img.n_frames):
                img.seek(frame)
                sizes_found.append(img.size)
            
            # Check for required sizes
            required_sizes = [(16, 16), (32, 32), (48, 48)]
            missing_sizes = [size for size in required_sizes if size not in sizes_found]
            
            if missing_sizes:
                return False
            else:
                return True

    except ImportError:
        return None
    except Exception as e:
        return None

if __name__=="__main__":
    app = QApplication(sys.argv)
    app.setStyle("Fusion")
    
    # Set application icon - using absolute path
    icon_path = os.path.abspath(os.path.join(os.path.dirname(sys.argv[0]), "calcforge.ico"))
    
    if os.path.exists(icon_path):
        # Verify icon file contents
        verify_icon_file(icon_path)
        
        icon = QIcon(icon_path)
        app.setWindowIcon(icon)
        # Force the icon to be set for Windows
        try:
            import ctypes
            ctypes.windll.shell32.SetCurrentProcessExplicitAppUserModelID("calcforge.4.0")
            # Set the icon for the taskbar
            import win32gui
            import win32con
            hwnd = win32gui.GetForegroundWindow()
            win32gui.SendMessage(hwnd, win32con.WM_SETICON, win32con.ICON_BIG, icon)
            win32gui.SendMessage(hwnd, win32con.WM_SETICON, win32con.ICON_SMALL, icon)
        except ImportError:
            pass
        except Exception as e:
            pass

    # Create and show calculator
    win = Calculator()
    if os.path.exists(icon_path):
        win.setWindowIcon(icon)  # Set icon again explicitly for the window
    win.show()
    
    # Force the window to get focus when launched (especially important for keyboard shortcuts)
    win.raise_()  # Bring window to front
    win.activateWindow()  # Activate the window
    win.setFocus()  # Set focus to the window
    
    # Additional Windows-specific focus forcing
    try:
        import ctypes
        from ctypes import wintypes
        
        # Get the window handle
        hwnd = int(win.winId())
        
        # Force the window to the foreground
        ctypes.windll.user32.SetForegroundWindow(hwnd)
        ctypes.windll.user32.BringWindowToTop(hwnd)
        
        # Additional method to ensure focus
        ctypes.windll.user32.SetActiveWindow(hwnd)
            
    except Exception as e:
        pass
    
    # Use our custom focus method to ensure editor gets focus too
    win.force_focus()
    
    # Also set focus after a short delay to ensure window is fully rendered
    # This is especially important when launched via keyboard shortcuts
    from PySide6.QtCore import QTimer
    def delayed_focus():
        win.force_focus()
        # Additional Windows-specific delayed focus
        try:
            import ctypes
            hwnd = int(win.winId())
            ctypes.windll.user32.SetForegroundWindow(hwnd)
        except:
            pass
    
    QTimer.singleShot(100, delayed_focus)  # 100ms delay
    
    app.exec()
        
        