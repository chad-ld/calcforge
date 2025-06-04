"""
CalcForge Core Engine - Backend Logic Extracted from Qt Application
Handles all calculation, evaluation, and data processing logic.
"""

import sys
import os
import json
import re
import math
import statistics
import time
import traceback
from pathlib import Path
from collections import Counter
from datetime import datetime, timedelta
import calendar

# Third-party imports
import pint
ureg = pint.UnitRegistry()

# Import constants
from constants import (
    FALLBACK_RATES, CURRENCY_ABBR, CURRENCY_DISPLAY,
    UNIT_ABBR, UNIT_DISPLAY, MATH_FUNCS, COLORS, LN_COLORS,
    FUNCTION_NAMES, DEFAULT_FPS, lcm
)

# Import syntax highlighter
from syntax_highlighter import SyntaxHighlighter

# Currency conversion support
try:
    import requests
    CURRENCY_API_AVAILABLE = True
except ImportError:
    CURRENCY_API_AVAILABLE = False


class TimecodeError(Exception):
    """Custom exception for timecode-related errors"""
    pass


class CalcForgeEngine:
    """
    Core calculation engine for CalcForge.
    Handles all mathematical operations, unit conversions, timecode calculations,
    date arithmetic, and cross-sheet references.
    """
    
    def __init__(self):
        self.worksheets = {}  # Store worksheet data
        self.current_sheet = 0
        self.ln_value_map = {}  # Store line number to value mappings
        self.cross_sheet_cache = {}  # Cache for cross-sheet references
        self.syntax_highlighter = SyntaxHighlighter()  # Syntax highlighter instance

        # Initialize global evaluation namespace
        self.globals = {
            "TC": self.TC,
            "AR": self.AR,
            "D": self.D,
            "truncate": self.truncate,
            "TR": self.truncate,
            **MATH_FUNCS
        }
    
    def evaluate_expression(self, expr, sheet_id=0, line_num=1):
        """
        Core expression evaluation method.
        
        Args:
            expr (str): The expression to evaluate
            sheet_id (int): ID of the worksheet
            line_num (int): Line number for context
            
        Returns:
            dict: Result containing value, unit (if applicable), and any errors
        """
        try:
            # Clean the expression
            expr = expr.strip()
            
            if not expr or expr.startswith(":::"):
                return {"value": "", "unit": "", "error": None}
            
            # Handle date arithmetic
            if expr.upper().startswith('D(') and expr.endswith(')'):
                date_expr = expr[2:-1]  # Remove D( and )
                result = self.handle_date_arithmetic(date_expr)
                if result is not None:
                    return {"value": result, "unit": "", "error": None}
            
            # Process LN references
            if re.search(r"\bLN(\d+)\b", expr, re.IGNORECASE):
                expr = self.process_ln_refs(expr, sheet_id)
            
            # Process cross-sheet references
            if re.search(r"\bS\..*?\.LN\d+\b", expr, re.IGNORECASE):
                expr = self.process_cross_sheet_refs(expr)
            
            # Handle unit conversion
            unit_result = self.handle_unit_conversion(expr)
            if unit_result is not None:
                return {"value": unit_result['value'], "unit": unit_result['unit'], "error": None}
            
            # Handle currency conversion
            currency_result = self.handle_currency_conversion(expr)
            if currency_result is not None:
                return {"value": currency_result['value'], "unit": currency_result['unit'], "error": None}
            
            # Handle statistical functions
            stat_result = self.handle_statistical_functions(expr, sheet_id)
            if stat_result is not None:
                return {"value": stat_result, "unit": "", "error": None}
            
            # Convert ^ to ** for proper exponentiation
            expr = expr.replace('^', '**')

            # Regular mathematical evaluation
            result = eval(expr, self.globals, {})
            
            # Format the result
            if isinstance(result, float):
                result = round(result, 6)
                if result.is_integer():
                    result = int(result)
            
            return {"value": result, "unit": "", "error": None}
            
        except Exception as e:
            error_msg = f"ERROR: {str(e)}"
            return {"value": "", "unit": "", "error": error_msg}
    
    def get_syntax_highlights(self, text):
        """
        Generate syntax highlighting data for frontend.
        Returns CSS class information instead of Qt formatting.

        Args:
            text (str): Text to highlight

        Returns:
            list: List of highlight ranges with CSS classes
        """
        return self.syntax_highlighter.highlight_text(text)
    
    def manage_cross_sheet_refs(self, sheets_data):
        """
        Handle cross-sheet references between worksheets.
        
        Args:
            sheets_data (dict): Dictionary of sheet_id -> sheet_content
        """
        self.worksheets = sheets_data
        self.cross_sheet_cache.clear()
        
        # Build cache for cross-sheet lookups
        for sheet_id, content in sheets_data.items():
            lines = content.split('\n')
            sheet_cache = {}
            
            for i, line in enumerate(lines):
                line = line.strip()
                if line and not line.startswith(":::"):
                    # Store line content with line number
                    sheet_cache[i + 1] = line
            
            self.cross_sheet_cache[sheet_id] = sheet_cache

    # =========================================================================
    # TIMECODE FUNCTIONS
    # =========================================================================

    def parse_timecode(self, tc_str):
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

    def timecode_to_frames(self, tc_str, fps):
        """Convert a timecode string to total frames"""
        if isinstance(tc_str, (int, float)):
            return int(tc_str)

        hours, minutes, seconds, frames = self.parse_timecode(tc_str)

        # Validate frame count against fps
        max_frames = int(fps) if fps == int(fps) else int(fps) + 1
        if frames >= max_frames:
            raise TimecodeError(f"Frame count {frames} exceeds maximum for {fps} fps (max: {max_frames-1})")

        if abs(fps - 29.97) < 0.01:
            # For 29.97 fps drop frame
            total_minutes = (60 * hours) + minutes
            total_frames = (hours * 3600 * 30) + (minutes * 60 * 30) + (seconds * 30) + frames
            drops = 2 * (total_minutes - total_minutes // 10)
            result = total_frames - drops
            return result

        elif abs(fps - 59.94) < 0.01:
            # For 59.94 fps drop frame
            total_minutes = (60 * hours) + minutes
            total_frames = (hours * 3600 * 60) + (minutes * 60 * 60) + (seconds * 60) + frames
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

    def frames_to_timecode(self, frame_count, fps):
        """Convert frame count to timecode string"""
        if frame_count < 0:
            sign = "-"
            frame_count = abs(frame_count)
        else:
            sign = ""

        if abs(fps - 29.97) < 0.01:
            # For 29.97 drop frame
            total_minutes = frame_count // (30 * 60)
            drops = 2 * (total_minutes - total_minutes // 10)
            real_frames = frame_count + drops

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

    def evaluate_timecode_expr(self, fps, expr):
        """Evaluate a timecode expression"""
        if isinstance(expr, (int, float)):
            return self.frames_to_timecode(int(expr), fps)

        # Normalize all timecode separators to colons
        expr = expr.replace('.', ':')

        # Add spaces around operators
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
                    frames = self.timecode_to_frames(token, fps)
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

        return self.frames_to_timecode(int(round(result)), fps)

    def TC(self, fps, *args):
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
                return self.frames_to_timecode(int(expr), fps)

            # If it's a single timecode without arithmetic, normalize separators and return frames
            if re.match(r'^\d{1,2}[:\.]\d{1,2}[:\.]\d{1,2}[:\.]\d{1,2}$', expr):
                expr = expr.replace('.', ':')
                frames = self.timecode_to_frames(expr, fps)
                return str(frames)

            # Handle timecode arithmetic
            return self.evaluate_timecode_expr(fps, expr)
        except Exception as e:
            raise TimecodeError(f"Error in TC function: {str(e)}")

    # =========================================================================
    # DATE ARITHMETIC FUNCTIONS
    # =========================================================================

    def parse_date(self, date_str):
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
            '%B %d, %Y',  # "July 12, 1985"
            '%B %d,%Y',   # "July 12,1985"
            '%b %d, %Y',  # "Jul 12, 1985"
            '%b %d,%Y',   # "Jul 12,1985"
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

    def add_business_days(self, start_date, days):
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

    def count_business_days(self, start_date, end_date):
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

    def handle_date_arithmetic(self, expr):
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
                        date1 = self.parse_date(groups[0].strip())
                        date2 = self.parse_date(groups[1].strip())
                        # Check if there's a W before the minus sign
                        if 'W-' in expr or 'W -' in expr:
                            days = self.count_business_days(date1, date2)
                            return {'value': days, 'unit': 'Business Days'}
                        else:
                            days = (date2 - date1).days
                            return {'value': days, 'unit': 'Days'}
                    except ValueError as e:
                        return None

                elif len(groups) == 3:  # Date arithmetic
                    try:
                        date = self.parse_date(groups[0].strip())
                        op = groups[1]
                        days = int(groups[2])

                        # Check if this is a business day calculation
                        if 'W' in expr:
                            if op == '+':
                                result = self.add_business_days(date, days)
                            else:
                                result = self.add_business_days(date, -days)
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
                        date = self.parse_date(groups[0].strip())
                        return date.strftime('%B %d, %Y')
                    except ValueError as e:
                        return None

        return None

    def D(self, *args):
        """Date arithmetic function"""
        if not args:
            return None

        # Join all arguments to handle expressions with spaces
        expr = ' '.join(str(arg) for arg in args)
        return self.handle_date_arithmetic(expr)

    # =========================================================================
    # ASPECT RATIO AND UTILITY FUNCTIONS
    # =========================================================================

    def AR(self, original, target):
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

    def truncate(self, value, decimals=2):
        """Rounds a number to specified decimal places"""
        if isinstance(value, str):
            # If it's a string expression, evaluate it first
            try:
                value = eval(value, self.globals, {})
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

    # =========================================================================
    # CURRENCY CONVERSION FUNCTIONS
    # =========================================================================

    def get_exchange_rate(self, from_currency, to_currency):
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

    def handle_currency_conversion(self, expr):
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
                    rate = self.get_exchange_rate(from_abbr, to_abbr)

                    if rate is not None:
                        result = value * rate
                        # Get display name for the target currency
                        display_currency = CURRENCY_DISPLAY.get(to_abbr, to_currency)
                        return {'value': result, 'unit': display_currency}
                except (ValueError, TypeError):
                    pass

        return None

    # =========================================================================
    # UNIT CONVERSION FUNCTIONS
    # =========================================================================

    def handle_unit_conversion(self, expr):
        """Handle unit conversion expressions using pint library"""
        # Pattern to match unit conversions like "5 feet to meters"
        pattern = r'^([\d.]+)\s+(.+?)\s+to\s+(.+?)$'
        match = re.match(pattern, expr.strip(), re.IGNORECASE)

        if match:
            value_str, from_unit, to_unit = match.groups()

            try:
                value = float(value_str)

                # Clean up unit names
                from_unit = from_unit.lower().strip()
                to_unit = to_unit.lower().strip()

                # Convert to standard abbreviations if needed
                from_unit_abbr = UNIT_ABBR.get(from_unit, from_unit)
                to_unit_abbr = UNIT_ABBR.get(to_unit, to_unit)

                # Create pint quantities
                quantity = ureg.Quantity(value, from_unit_abbr)
                converted = quantity.to(to_unit_abbr)

                # Get display name for the target unit
                display_unit = UNIT_DISPLAY.get(to_unit_abbr, to_unit)

                return {'value': converted.magnitude, 'unit': display_unit}

            except Exception as e:
                # If pint conversion fails, return None to try other evaluation methods
                pass

        return None

    # =========================================================================
    # STATISTICAL FUNCTIONS
    # =========================================================================

    def handle_statistical_functions(self, expr, sheet_id=0):
        """Handle statistical functions like sum, mean, etc."""
        # Pattern to match statistical functions
        stat_pattern = r'^(sum|mean|median|mode|min|max|count|product|variance|stdev|std|range|geomean|harmmean|sumsq|perc5|perc95|meanfps)\s*\(\s*(.+?)\s*\)$'
        match = re.match(stat_pattern, expr.strip(), re.IGNORECASE)

        if match:
            func_name = match.group(1).lower()
            range_expr = match.group(2)

            # Get values from the range expression
            values = self.get_values_from_range(range_expr, sheet_id)

            if not values:
                return 0

            # Apply the statistical function
            try:
                if func_name == 'sum':
                    return sum(values)
                elif func_name == 'mean':
                    return statistics.mean(values)
                elif func_name == 'median':
                    return statistics.median(values)
                elif func_name == 'mode':
                    return statistics.mode(values)
                elif func_name == 'min':
                    return min(values)
                elif func_name == 'max':
                    return max(values)
                elif func_name == 'count':
                    return len(values)
                elif func_name == 'product':
                    result = 1
                    for v in values:
                        result *= v
                    return result
                elif func_name in ['variance']:
                    return statistics.variance(values) if len(values) > 1 else 0
                elif func_name in ['stdev', 'std']:
                    return statistics.stdev(values) if len(values) > 1 else 0
                elif func_name == 'range':
                    return max(values) - min(values)
                elif func_name == 'geomean':
                    return statistics.geometric_mean(values)
                elif func_name == 'harmmean':
                    return statistics.harmonic_mean(values)
                elif func_name == 'sumsq':
                    return sum(v * v for v in values)
                elif func_name == 'perc5':
                    return statistics.quantiles(values, n=20)[0] if len(values) > 1 else values[0]
                elif func_name == 'perc95':
                    return statistics.quantiles(values, n=20)[18] if len(values) > 1 else values[0]
                elif func_name == 'meanfps':
                    # Special mean function that considers frame rates
                    return statistics.mean(values)
            except Exception as e:
                return 0

        return None

    def get_values_from_range(self, range_expr, sheet_id=0):
        """Extract numeric values from a range expression"""
        values = []

        # Handle different range formats
        if '-' in range_expr and not range_expr.startswith('-'):
            # Range format like "1-5"
            parts = range_expr.split('-')
            if len(parts) == 2:
                try:
                    start = int(parts[0].strip())
                    end = int(parts[1].strip())
                    for i in range(start, end + 1):
                        value = self.get_line_value(i, sheet_id)
                        if value is not None:
                            values.append(value)
                except ValueError:
                    pass
        elif ',' in range_expr:
            # Comma-separated format like "1,3,5"
            parts = range_expr.split(',')
            for part in parts:
                try:
                    line_num = int(part.strip())
                    value = self.get_line_value(line_num, sheet_id)
                    if value is not None:
                        values.append(value)
                except ValueError:
                    pass
        elif range_expr.lower() in ['above', 'below']:
            # Special keywords for relative ranges
            # This would need current line context to implement properly
            pass
        else:
            # Single line number
            try:
                line_num = int(range_expr.strip())
                value = self.get_line_value(line_num, sheet_id)
                if value is not None:
                    values.append(value)
            except ValueError:
                pass

        return values

    def get_line_value(self, line_num, sheet_id=0):
        """Get the numeric value from a specific line"""
        # This would need to be implemented based on how line values are stored
        # For now, return from ln_value_map if available
        return self.ln_value_map.get(line_num)

    # =========================================================================
    # LN REFERENCE PROCESSING
    # =========================================================================

    def process_ln_refs(self, expr, sheet_id=0):
        """Process LN references in expressions"""
        def replace_ln_ref(match):
            ln_num = int(match.group(1))
            value = self.ln_value_map.get(ln_num)
            if value is not None:
                return str(value)
            else:
                return "0"  # Default to 0 if line not found

        # Replace LN references with their values
        return re.sub(r'\bLN(\d+)\b', replace_ln_ref, expr, flags=re.IGNORECASE)

    def process_cross_sheet_refs(self, expr):
        """Process cross-sheet references in expressions"""
        def replace_sheet_ref(match):
            sheet_name = match.group(1).lower()
            ln_num = int(match.group(2))

            # Look up value in cross-sheet cache
            if sheet_name in self.cross_sheet_cache:
                sheet_cache = self.cross_sheet_cache[sheet_name]
                if ln_num in sheet_cache:
                    # This would need proper evaluation of the referenced line
                    return "0"  # Placeholder for now

            return "0"  # Default to 0 if reference not found

        # Replace cross-sheet references with their values
        return re.sub(r'\bS\.([^.]+)\.LN(\d+)\b', replace_sheet_ref, expr, flags=re.IGNORECASE)
