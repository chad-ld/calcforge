import sys, os, json, re, math
from pathlib import Path
from collections import Counter
from datetime import datetime, timedelta
import calendar
import statistics  # Add statistics import at top level

import pint
ureg = pint.UnitRegistry()

# Add currency conversion support
try:
    import requests
    CURRENCY_API_AVAILABLE = True
except ImportError:
    CURRENCY_API_AVAILABLE = False

# Currency exchange rates (fallback if API fails)
FALLBACK_RATES = {
    'usd': 1.0,  # Base currency
    'eur': 0.85,
    'gbp': 0.73,
    'jpy': 110.0,
    'cad': 1.25,
    'aud': 1.35,
    'chf': 0.92,
    'cny': 6.45,
    'inr': 74.5,
    'krw': 1180.0,
    'mxn': 20.1,
    'brl': 5.2,
    'rub': 73.5,
    'sek': 8.6,
    'nok': 8.4,
    'dkk': 6.3,
    'pln': 3.9,
    'czk': 21.8,
    'huf': 297.0,
    'twd': 27.8,
    'sgd': 1.34,
    'nzd': 1.42,
    'zar': 14.2,
    'ils': 3.2,
    'thb': 31.5,
    'hkd': 7.8,
}

def lcm(a, b):
    """Calculate the Least Common Multiple of two numbers"""
    return abs(a * b) // math.gcd(a, b)

def parse_date(date_str):
    """Parse a date string in various formats starting with 'd', 'D', 'd.', or 'D.'"""
    # Remove the prefix (either 'd/D' or 'd./D.') and clean up whitespace
    date_str = re.sub(r'^[dD]\.?\s*', '', date_str.strip())
    
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
    """Handle date arithmetic expressions"""
    # First, store the original expression for later
    original_expr = expr
    
    # Match date arithmetic patterns, allowing for optional period after d/D
    date_patterns = [
        # Two dates with subtraction - handle spaces in dates
        r'([dD]\.?(?:[A-Za-z]+\s+\d+,\s*\d{4}|\d[^+\-W]*?))\s*(?:W)?\s*-\s*([dD]\.?(?:[A-Za-z]+\s+\d+,\s*\d{4}|\d[^+\-]+))',  # Optional W before -
        # Date plus/minus days - handle spaces and optional W
        r'([dD]\.?(?:[A-Za-z]+\s+\d+,\s*\d{4}|\d[^+\-W]*?))\s*W\s*([+\-])\s*(\d+)',  # With W
        r'([dD]\.?(?:[A-Za-z]+\s+\d+,\s*\d{4}|\d[^+\-W]*?))\s*([+\-])\s*(\d+)',      # Without W
        # Single date
        r'([dD]\.?(?:[A-Za-z]+\s+\d+,\s*\d{4}|\d[^+\-]+))'
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
                    else:
                        days = (date2 - date1).days
                    return {'value': days, 'unit': 'Business Days' if 'W' in expr else 'Days'}
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

# Add all math functions to globals
MATH_FUNCS = {
    # Trigonometric functions
    'sin': math.sin, 'cos': math.cos, 'tan': math.tan,
    'asin': math.asin, 'acos': math.acos, 'atan': math.atan,
    'sinh': math.sinh, 'cosh': math.cosh, 'tanh': math.tanh,
    'asinh': math.asinh, 'acosh': math.acosh, 'atanh': math.atanh,
    'degrees': math.degrees, 'radians': math.radians,
    # Power and logarithmic functions
    'sqrt': math.sqrt, 'pow': math.pow, 'exp': math.exp,
    'log': math.log, 'log10': math.log10, 'log2': math.log2,
    # Other mathematical functions
    'ceil': math.ceil, 'floor': math.floor, 'abs': abs,
    'factorial': math.factorial, 'gcd': math.gcd, 'lcm': lcm,
    # Constants
    'pi': math.pi, 'e': math.e
}

class TimecodeError(Exception):
    pass

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

def preprocess_expression(expr):
    """Pre-process expression to handle padded numbers and other special cases"""
    # Debug: Print original expression if it contains TC
    if 'TC(' in expr:
        print(f"DEBUG: Original TC expression: {expr}")
    
    # Handle timecode arithmetic first (BEFORE comma removal to preserve function arguments)
    tc_match = re.match(r'TC\((.*?)\)', expr)
    if tc_match:
        tc_args = tc_match.group(1)
        print(f"DEBUG: TC args extracted: '{tc_args}'")
        
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
        
        print(f"DEBUG: TC parts after splitting: {parts}")
        
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
        
        print(f"DEBUG: TC processed parts: {processed_parts}")
        
        # Reconstruct the TC call
        expr = f"TC({','.join(processed_parts)})"
        print(f"DEBUG: TC reconstructed expression: {expr}")
    
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
    def remove_thousands_commas(match):
        number_str = match.group(0)
        # Remove all commas from the number
        return number_str.replace(',', '')
    
    # Pattern to match numbers with commas like 1,234 or 1,234.56
    # Use negative lookbehind to avoid matching inside function calls
    # This pattern avoids matching numbers that come after an opening parenthesis
    comma_number_pattern = r'(?<!\()\b\d{1,3}(?:,\d{3})*(?:\.\d+)?\b(?![^()]*\))'
    expr = re.sub(comma_number_pattern, remove_thousands_commas, expr)
    
    # Replace numbers with leading zeros outside of timecodes and quoted strings
    def repl_num(m):
        # Don't replace if it's part of a timecode
        if re.match(r'\d{1,2}[:.]\d{1,2}[:.]\d{1,2}[:.]\d{1,2}', m.string[max(0, m.start()-8):m.end()+8]):
            return m.group(0)
        # Don't replace if it's inside quotes
        before_match = m.string[:m.start()]
        after_match = m.string[m.end():]
        quote_count_before = before_match.count('"') - before_match.count('\\"')
        if quote_count_before % 2 == 1:  # We're inside quotes
            return m.group(0)
        return str(int(m.group(1)))
    
    expr = re.sub(r'\b0+(\d+)\b', repl_num, expr)
    
    # Debug: Print final expression if it contains TC
    if 'TC(' in expr:
        print(f"DEBUG: Final TC expression: {expr}")
    
    return expr

# Add TC function and math functions to evaluation namespace
GLOBALS = {"TC": TC, "AR": AR, **MATH_FUNCS}

from PySide6.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QHBoxLayout, QPlainTextEdit,
    QTextEdit, QSplitter, QPushButton, QMessageBox, QTabWidget, QInputDialog,
    QToolTip, QCompleter, QListWidget, QCheckBox, QDialog
)
from PySide6.QtGui import (
    QFont, QSyntaxHighlighter, QTextCharFormat, QColor,
    QTextCursor, QPainter, QPalette, QTextBlockUserData, QStandardItemModel,
    QStandardItem, QIcon, QPen
)
from PySide6.QtCore import (
    Qt, QTimer, QRegularExpression, QSize, QRect, Slot, QSettings, QEvent,
    QStringListModel, QObject
)

# Forward declare Calculator class for type hints
class Calculator(QWidget):
    pass

# Update the unit abbreviation mapping
UNIT_ABBR = {
    # Distance
    'meter': 'm', 'meters': 'm', 'm': 'm',
    'kilometer': 'km', 'kilometers': 'km', 'km': 'km',
    'mile': 'mi', 'miles': 'mi', 'mi': 'mi',
    'yard': 'yd', 'yards': 'yd', 'yd': 'yd',
    'foot': 'ft', 'feet': 'ft', 'ft': 'ft',
    'inch': 'in', 'inches': 'in', 'in': 'in',
    
    # Weight
    'pound': 'lb', 'pounds': 'lb', 'lb': 'lb', 'lbs': 'lb',
    'kilogram': 'kg', 'kilograms': 'kg', 'kg': 'kg',
    'gram': 'g', 'grams': 'g', 'g': 'g',
    'ounce': 'oz', 'ounces': 'oz', 'oz': 'oz',
    'ton': 't', 'tons': 't', 't': 't',
    
    # Volume
    'liter': 'L', 'liters': 'L', 'litre': 'L', 'litres': 'L', 'L': 'L',
    'gallon': 'gal', 'gallons': 'gal', 'gal': 'gal',
    'quart': 'qt', 'quarts': 'qt', 'qt': 'qt',
    'pint': 'pt', 'pints': 'pt', 'pt': 'pt',
    'cup': 'cup', 'cups': 'cup',
    'milliliter': 'mL', 'milliliters': 'mL', 'mL': 'mL',
}

# Mapping of abbreviated units to their full spelling for display
UNIT_DISPLAY = {
    # Distance
    'm': 'meters',
    'km': 'kilometers',
    'mi': 'miles',
    'yd': 'yards',
    'ft': 'feet',
    'in': 'inches',
    
    # Weight
    'lb': 'pounds',
    'kg': 'kilograms',
    'g': 'grams',
    'oz': 'ounces',
    't': 'tons',
    
    # Volume
    'L': 'liters',
    'gal': 'gallons',
    'qt': 'quarts',
    'pt': 'pints',
    'cup': 'cups',
    'mL': 'milliliters',
}

# Currency abbreviation mapping
CURRENCY_ABBR = {
    'dollar': 'usd', 'dollars': 'usd', 'usd': 'usd', 'us dollar': 'usd', 'us dollars': 'usd',
    'euro': 'eur', 'euros': 'eur', 'eur': 'eur',
    'pound': 'gbp', 'pounds': 'gbp', 'gbp': 'gbp', 'british pound': 'gbp', 'british pounds': 'gbp',
    'yen': 'jpy', 'jpy': 'jpy', 'japanese yen': 'jpy',
    'canadian dollar': 'cad', 'canadian dollars': 'cad', 'cad': 'cad',
    'australian dollar': 'aud', 'australian dollars': 'aud', 'aud': 'aud',
    'swiss franc': 'chf', 'swiss francs': 'chf', 'chf': 'chf',
    'yuan': 'cny', 'cny': 'cny', 'chinese yuan': 'cny', 'rmb': 'cny',
    'rupee': 'inr', 'rupees': 'inr', 'inr': 'inr', 'indian rupee': 'inr', 'indian rupees': 'inr',
    'won': 'krw', 'krw': 'krw', 'korean won': 'krw',
    'peso': 'mxn', 'pesos': 'mxn', 'mxn': 'mxn', 'mexican peso': 'mxn', 'mexican pesos': 'mxn',
    'real': 'brl', 'reais': 'brl', 'brl': 'brl', 'brazilian real': 'brl',
    'ruble': 'rub', 'rubles': 'rub', 'rub': 'rub', 'russian ruble': 'rub', 'russian rubles': 'rub',
    'krona': 'sek', 'kronor': 'sek', 'sek': 'sek', 'swedish krona': 'sek',
    'krone': 'nok', 'kroner': 'nok', 'nok': 'nok', 'norwegian krone': 'nok',
    'dkk': 'dkk', 'danish krone': 'dkk',
    'zloty': 'pln', 'pln': 'pln', 'polish zloty': 'pln',
    'koruna': 'czk', 'czk': 'czk', 'czech koruna': 'czk',
    'forint': 'huf', 'huf': 'huf', 'hungarian forint': 'huf',
    'twd': 'twd', 'taiwan dollar': 'twd', 'new taiwan dollar': 'twd',
    'sgd': 'sgd', 'singapore dollar': 'sgd',
    'nzd': 'nzd', 'new zealand dollar': 'nzd',
    'rand': 'zar', 'zar': 'zar', 'south african rand': 'zar',
    'shekel': 'ils', 'shekels': 'ils', 'ils': 'ils', 'israeli shekel': 'ils',
    'baht': 'thb', 'thb': 'thb', 'thai baht': 'thb',
    'hkd': 'hkd', 'hong kong dollar': 'hkd',
}

# Currency display names
CURRENCY_DISPLAY = {
    'usd': 'US Dollars',
    'eur': 'Euros',
    'gbp': 'British Pounds',
    'jpy': 'Japanese Yen',
    'cad': 'Canadian Dollars',
    'aud': 'Australian Dollars',
    'chf': 'Swiss Francs',
    'cny': 'Chinese Yuan',
    'inr': 'Indian Rupees',
    'krw': 'Korean Won',
    'mxn': 'Mexican Pesos',
    'brl': 'Brazilian Real',
    'rub': 'Russian Rubles',
    'sek': 'Swedish Kronor',
    'nok': 'Norwegian Kroner',
    'dkk': 'Danish Kroner',
    'pln': 'Polish Zloty',
    'czk': 'Czech Koruna',
    'huf': 'Hungarian Forint',
    'twd': 'Taiwan Dollars',
    'sgd': 'Singapore Dollars',
    'nzd': 'New Zealand Dollars',
    'zar': 'South African Rand',
    'ils': 'Israeli Shekels',
    'thb': 'Thai Baht',
    'hkd': 'Hong Kong Dollars',
}

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

class LineData(QTextBlockUserData):
    def __init__(self, id):
        super().__init__()
        self.id = id

class LineNumberArea(QWidget):
    def __init__(self, editor):
        super().__init__(editor)
        self.editor = editor
    def sizeHint(self):
        return QSize(self.editor.lineNumberAreaWidth(), 0)
    def paintEvent(self, event):
        self.editor.lineNumberAreaPaintEvent(event)

class FormulaHighlighter(QSyntaxHighlighter):
    def __init__(self, document):
        super().__init__(document)
        # Base colors for syntax
        self.formats = {
            'number': self._fmt("#FFFFFF"),
            'operator': self._fmt("#4DA6FF"),
            'paren': self._fmt("#6FCF97"),
            'unmatched': self._fmt("#FF5C5C"),
            'comment': self._fmt("#7ED321"),
        }
        
        # Color palette for LN variables - colors chosen to be readable on dark background
        self.ln_colors = [
            "#FF9999", "#99FF99", "#9999FF", "#FFFF99", "#FF99FF", "#99FFFF",
            "#FFB366", "#B3FF66", "#66FFB3", "#66B3FF", "#B366FF", "#FF66B3"
        ]
        
        # Store persistent LN colors
        self.persistent_ln_colors = {}
        
    def _fmt(self, color, alpha=255):
        fmt = QTextCharFormat()
        fmt.setForeground(QColor(color))
        return fmt
        
    def get_darker_color(self, color, factor=0.3):
        """Return a darker version of the given color for background highlighting"""
        c = QColor(color)
        h, s, v, a = c.getHsv()
        return QColor.fromHsv(h, s, int(v * factor), a)
        
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

class AutoCompleteList(QListWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowFlags(Qt.ToolTip | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint)
        self.setAttribute(Qt.WA_ShowWithoutActivating)
        self.setFocusPolicy(Qt.NoFocus)
        self.setMouseTracking(True)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
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
                padding: 4px;
            }
            QListWidget::item:selected {
                background-color: #4477ff;
            }
            QListWidget::item:hover {
                background-color: #3a3a3d;
            }
        """)

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
            k = event.key()
            
            # Check if we have a selection when Ctrl key events come in
            cursor = self.editor.textCursor()
            has_selection = cursor.hasSelection()
            
            if has_selection and k == 16777249:  # This is the Ctrl key
                return True
            
            if has_selection and ctrl:
                if k == Qt.Key_C:
                    # Handle Ctrl+C directly here to prevent Qt from clearing selection
                    selected_text = cursor.selectedText()
                    if selected_text:
                        # Convert Qt's special line breaks to regular line breaks
                        text = selected_text.replace('\u2029', '\n').replace('\u2028', '\n')
                        clipboard = QApplication.clipboard()
                        clipboard.setText(text)
                        return True  # Event handled, don't pass to Qt
                        
                elif k in (Qt.Key_Left, Qt.Key_Right, Qt.Key_Up, Qt.Key_Down):
                    # For Ctrl+arrow keys with an existing selection, do nothing to preserve selection
                    return True  # Block the event to preserve selection
        
        # For all other events, pass through normally
        return False

class FormulaEditor(QPlainTextEdit):
    def __init__(self, parent):
        super().__init__(parent)
        self.parent = parent
        self.next_line_id = 1
        self.default_font_size = 14
        self.current_font_size = self.parent.settings.value('font_size', self.default_font_size, type=int)
        
        # Add highlight selection tracking
        self.current_highlight = None  # Store current highlight selection
        
        # Add truncate function to the editor instance
        self.truncate = self.truncate_func
        
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
        """Update the set of lines that need separators above them"""
        self.separator_lines.clear()
        lines = self.toPlainText().split('\n')
        
        # List of functions that operate on lines above
        line_functions = {'sum', 'mean', 'median', 'mode', 'min', 'max', 'range', 
                         'count', 'product', 'variance', 'stdev', 'std', 'geomean', 
                         'harmmean', 'sumsq', 'perc5', 'perc95'}
        
        for i, line in enumerate(lines):
            # Check for function calls that operate on lines above
            match = re.match(r'(\w+)\s*\((.*?)\)', line.strip())
            if match:
                func_name = match.group(1).lower()
                args = match.group(2).strip()
                
                if func_name in line_functions:
                    # If it's a line function, add a separator above this line
                    self.separator_lines.add(i)
        
        # Force a viewport update to show the new separators
        self.viewport().update()

    def setup_autocompletion(self):
        # Basic functions and commands
        self.base_completions = {
            'TR': 'TR(value, decimals)',
            'truncate': 'truncate(value, decimals)',
            'sqrt': 'sqrt(value)',
            'sin': 'sin(value)',
            'cos': 'cos(value)',
            'tan': 'tan(value)',
            'asin': 'asin(value)',
            'acos': 'acos(value)',
            'atan': 'atan(value)',
            'sinh': 'sinh(value)',
            'cosh': 'cosh(value)',
            'tanh': 'tanh(value)',
            'asinh': 'asinh(value)',
            'acosh': 'acosh(value)',
            'atanh': 'atanh(value)',
            'log': 'log(value)',
            'log10': 'log10(value)',
            'log2': 'log2(value)',
            'exp': 'exp(value)',
            'pow': 'pow(base, exponent)',
            'ceil': 'ceil(value)',
            'floor': 'floor(value)',
            'abs': 'abs(value)',
            'factorial': 'factorial(value)',
            'gcd': 'gcd(a, b)',
            'lcm': 'lcm(a, b)',
            'TC': 'TC(fps, timecode)',
            'AR': 'AR(1920x1080, ?x2000)',
            'pi': 'pi',
            'e': 'e',
            # Add special commands
            'sum': 'sum(start-end)',
            'mean': 'mean(start-end)',
            'median': 'median(start-end)',
            'mode': 'mode(start-end)',
            'min': 'min(start-end)',
            'max': 'max(start-end)',
            'range': 'range(start-end)',
            'count': 'count(start-end)',
            'product': 'product(start-end)',
            'variance': 'variance(start-end)',
            'stdev': 'stdev(start-end)',
            'std': 'std(start-end)',
            'geomean': 'geomean(start-end)',
            'harmmean': 'harmmean(start-end)',
            'sumsq': 'sumsq(start-end)',
            'perc5': 'perc5(start-end)',
            'perc95': 'perc95(start-end)'
        }

    def truncate_func(self, value, decimals=2):
        """Rounds a number to specified decimal places"""
        if isinstance(value, str):
            # If it's a string expression, evaluate it first
            try:
                value = eval(value, {"truncate": self.truncate_func, **GLOBALS}, {})
            except:
                return value
        if isinstance(value, dict) and 'value' in value:
            # Handle unit conversion results
            return {'value': round(value['value'] * (10 ** decimals)) / (10 ** decimals),
                   'unit': value['unit']}
        if not isinstance(value, (int, float)):
            return value
        factor = 10 ** decimals
        return round(value * factor) / factor

    def get_word_under_cursor(self):
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
        completions = []
        
        # Skip completions for LN references
        if prefix.lower().startswith('ln'):
            return []
        
        # Get the current line text to check for currency conversion context
        cursor = self.textCursor()
        line_text = cursor.block().text()
        cursor_pos = cursor.positionInBlock()
        
        # Check if we're in a currency conversion context
        # Pattern: number + currency + "to" + partial_currency
        currency_pattern = r'([\d.]+)\s+(\w+)\s+to\s+(\w*)$'
        match = re.search(currency_pattern, line_text[:cursor_pos], re.IGNORECASE)
        
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
        match = re.search(source_currency_pattern, line_text[:cursor_pos], re.IGNORECASE)
        
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
            
        # Regular function completions
        for key, value in self.base_completions.items():
            if key.lower().startswith(prefix.lower()):
                completions.append(value)

        return sorted(completions)

    def on_cursor_position_changed(self):
        # Store current cursor and scroll position
        cursor = self.textCursor()
        scrollbar = self.verticalScrollBar()
        current_scroll = scrollbar.value()
        
        # Always update the current line tracking
        self._last_line = cursor.blockNumber()
        self._current_block = cursor.block()
        
        # Always highlight the current line
        self.highlightCurrentLine()
        
        # Trigger scroll synchronization to keep results in sync
        if hasattr(self.parent, '_sync_editor_to_results'):
            self.parent._sync_editor_to_results(current_scroll)
        
        # Hide completion list if there's a selection
        if cursor.hasSelection():
            self.completion_list.hide()
            
        # Only restore cursor position if we don't have a user selection
        # to avoid interfering with manual text selection
        if not cursor.hasSelection():
            self.setTextCursor(cursor)

    def complete_text(self, item=None):
        if item is None:
            item = self.completion_list.currentItem()
        if item is None:
            return

        completion_text = item.text()
        cursor = self.textCursor()
        
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
            cursor.insertText(completion_text)
            
        # Move cursor inside parentheses if present
        if '(' in completion_text:
            pos = cursor.position()
            cursor.movePosition(QTextCursor.Left, QTextCursor.MoveAnchor, 
                              len(completion_text) - completion_text.rindex('(') - 1)
            cursor.setPosition(pos - 1)
            
        self.setTextCursor(cursor)
        self.completion_list.hide()

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

    def lineNumberAreaWidth(self):
        digits = len(str(max(1, self.blockCount())))
        return 3 + self.fontMetrics().horizontalAdvance('9') * digits

    def updateLineNumberAreaWidth(self, _):
        self.setViewportMargins(self.lineNumberAreaWidth(), 0, 0, 0)

    def calculate_subexpression(self, expr):
        """Calculate the result of a subexpression"""
        try:
            # Handle numbers with leading zeros
            expr = re.sub(r'\b0+(\d+)\b', r'\1', expr)
            
            # Process LN references if present
            if re.search(r"\bLN(\d+)\b", expr):
                expr = self.process_ln_refs(expr)
            
            # Handle the expression evaluation using the local truncate function
            result = eval(expr, {"truncate": self.truncate_func, **GLOBALS}, {})
            
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
        """Replace LN references and cross-sheet references with their values"""
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

        # First capitalize s. to S. for consistency
        expr = re.sub(r'\bs\.(.*?)\.ln', lambda m: f"S.{m.group(1)}.LN", expr, flags=re.IGNORECASE)
        # Then capitalize ln to LN in all cases
        expr = re.sub(r'\bln(\d+)\b', lambda m: f"LN{m.group(1)}", expr, flags=re.IGNORECASE)
        
        # Pattern for both cross-sheet and regular references
        # Group 1 and 2 will be None for regular LN refs
        pattern = r'\b(S\.)(.*?)\.LN(\d+)\b|\bLN(\d+)\b'
        
        # Keep replacing until no more changes
        prev_expr = None
        while prev_expr != expr:
            prev_expr = expr
            expr = re.sub(pattern, repl, expr)
            
        return expr

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

                # Handle LN reference tooltips (keep existing LN tooltip logic)
                for match in re.finditer(r'\bLN(\d+)\b', text):
                    start, end = match.span()
                    if start <= pos <= end:
                        found_operator = True
                        ln_id = int(match.group(1))
                        val = self.ln_value_map.get(ln_id)
                        if val is not None:
                            display = f"LN{ln_id} = {val}"
                        else:
                            display = f"LN{ln_id} not found"
                        QToolTip.showText(event.globalPosition().toPoint(), display, self)
                        # Don't return True here - let the mouse event continue for text selection
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
        while block.isValid() and top <= event.rect().bottom():
            if block.isVisible() and bottom >= event.rect().top():
                txt = block.text().strip()
                data = block.userData()
                label = "C" if txt.startswith(":::") else str(data.id if data else block.blockNumber()+1)
                color = "#7ED321" if txt.startswith(":::") else "#888"
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

    def highlightCurrentLine(self):
        """Highlight the current line and maintain any operator highlights"""
        selections = []
        results_selections = []
        
        # Clear all cross-sheet highlights first
        self.clear_cross_sheet_highlights()
        
        # Add current line highlight - but preserve user selections
        sel = QTextEdit.ExtraSelection()
        sel.format.setBackground(QColor(65, 65, 66))  # Main line highlight color
        sel.format.setProperty(QTextCharFormat.FullWidthSelection, True)
        sel.cursor = self.textCursor()
        # Don't clear selection if user has manually selected text
        if not sel.cursor.hasSelection():
            sel.cursor.clearSelection()
        else:
            # If user has a selection, create a new cursor for line highlighting
            # that doesn't interfere with the user's selection
            sel.cursor = QTextCursor(sel.cursor.block())
        selections.append(sel)
        
        # Add operator highlight if it exists
        if self.current_highlight:
            selections.append(self.current_highlight)
        
        # Add result line highlight - use block number for synchronization
        if hasattr(self.parent, 'results'):
            block_number = self.textCursor().blockNumber()
            results_block = self.parent.results.document().findBlockByNumber(block_number)
            if results_block.isValid():
                results_cursor = QTextCursor(results_block)
                sel_result = QTextEdit.ExtraSelection()
                sel_result.format.setBackground(QColor(65, 65, 66))  # Match editor highlight color
                sel_result.format.setProperty(QTextCharFormat.FullWidthSelection, True)
                sel_result.cursor = results_cursor
                results_selections.append(sel_result)
        
        # Get LN references from current line
        current_line_text = self.textCursor().block().text()
        # Make the regex case-insensitive and include cross-sheet references
        ln_matches = list(re.finditer(r'\b(?:s\.(.*?)\.)?ln(\d+)\b', current_line_text, re.IGNORECASE))
        
        calculator = self.get_calculator()
        
        # Collect cross-sheet highlights by sheet
        cross_sheet_highlights = {}
        cross_sheet_results_highlights = {}  # Add tracking for results highlights
        
        # Highlight referenced lines with matching colors
        for match in ln_matches:
            sheet_name = match.group(1)  # Will be None for regular LN refs
            ln_id = int(match.group(2))
            # Get the color used for this LN reference
            ln_color = self.highlighter.get_ln_color(ln_id)
            # Create darker version for background
            bg_color = self.highlighter.get_darker_color(ln_color)
            
            if sheet_name and calculator:  # Cross-sheet reference
                # Find the referenced sheet
                for i in range(calculator.tabs.count()):
                    if calculator.tabs.tabText(i).lower() == sheet_name.lower():
                        other_sheet = calculator.tabs.widget(i)
                        # Find and highlight the referenced line in the other sheet
                        doc = other_sheet.editor.document()
                        for j in range(doc.blockCount()):
                            blk = doc.findBlockByNumber(j)
                            user_data = blk.userData()
                            if isinstance(user_data, LineData) and user_data.id == ln_id:
                                # Create highlight for cross-sheet reference (editor column)
                                highlight_cursor = QTextCursor(blk)
                                sel_ref = QTextEdit.ExtraSelection()
                                sel_ref.format.setBackground(bg_color)
                                sel_ref.format.setProperty(QTextCharFormat.FullWidthSelection, True)
                                sel_ref.cursor = highlight_cursor
                                
                                # Collect highlights by sheet
                                if other_sheet not in cross_sheet_highlights:
                                    cross_sheet_highlights[other_sheet] = []
                                cross_sheet_highlights[other_sheet].append(sel_ref)
                                
                                # Also highlight the results column in the other sheet
                                if hasattr(other_sheet, 'results'):
                                    results_block = other_sheet.results.document().findBlockByNumber(j)
                                    if results_block.isValid():
                                        results_cursor = QTextCursor(results_block)
                                        sel_result = QTextEdit.ExtraSelection()
                                        sel_result.format.setBackground(bg_color)
                                        sel_result.format.setProperty(QTextCharFormat.FullWidthSelection, True)
                                        sel_result.cursor = results_cursor
                                        
                                        # Collect results highlights by sheet
                                        if other_sheet not in cross_sheet_results_highlights:
                                            cross_sheet_results_highlights[other_sheet] = []
                                        cross_sheet_results_highlights[other_sheet].append(sel_result)
                                break
            else:  # Regular LN reference
                # Find and highlight the referenced line
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
                        
                        # Highlight in results - use the same block number
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
        
        # Apply highlights to current sheet
        self.setExtraSelections(selections)
        if hasattr(self.parent, 'results'):
            self.parent.results.setExtraSelections(results_selections)
        
        # Apply cross-sheet highlights (both editor and results columns)
        for other_sheet, highlights in cross_sheet_highlights.items():
            other_sheet.editor.setExtraSelections(highlights)
            # Also apply results highlights if available
            if other_sheet in cross_sheet_results_highlights and hasattr(other_sheet, 'results'):
                other_sheet.results.setExtraSelections(cross_sheet_results_highlights[other_sheet])

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
        """Expand selection to include contents within parentheses or the parentheses themselves"""
        cursor = self.textCursor()
        text = cursor.block().text()
        block_pos = cursor.block().position()
        
        # Get current cursor position or selection
        if cursor.hasSelection():
            sel_start = cursor.selectionStart() - block_pos
            sel_end = cursor.selectionEnd() - block_pos
            pos = sel_start  # Use start of selection as reference point
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
        
        # Sort pairs by size (inner to outer)
        pairs.sort(key=lambda p: p[1] - p[0])
        
        # If we have a current selection, try to find if it matches any expression
        if cursor.hasSelection():
            # First check if we're selecting an arithmetic expression that's inside parentheses
            for start, end in pairs:
                # If our selection is inside these parentheses
                if start < sel_start and end > sel_end:
                    # And if we're not already selecting up to the parentheses
                    if start + 1 != sel_start or end != sel_end:
                        # Select up to the parentheses boundaries
                        cursor.setPosition(block_pos + start + 1)
                        cursor.setPosition(block_pos + end, QTextCursor.KeepAnchor)
                        self.setTextCursor(cursor)
                        return
                # If we're selecting exactly the content between parentheses
                elif start + 1 == sel_start and end == sel_end:
                    # Expand to include the parentheses
                    cursor.setPosition(block_pos + start)
                    cursor.setPosition(block_pos + end + 1, QTextCursor.KeepAnchor)
                    self.setTextCursor(cursor)
                    return
        
        # If we don't have a selection or couldn't expand it, find the innermost expression
        smallest_size = float('inf')
        smallest_pair = None
        
        # Look for the smallest pair containing the cursor
        for start, end in pairs:
            size = end - start
            if start < pos and end >= pos and size < smallest_size:
                smallest_size = size
                smallest_pair = (start, end)
            elif start + 1 <= pos and end >= pos and size < smallest_size:
                smallest_size = size
                smallest_pair = (start, end)
        
        if smallest_pair:
            start, end = smallest_pair
            # First select the contents
            cursor.setPosition(block_pos + start + 1)
            cursor.setPosition(block_pos + end, QTextCursor.KeepAnchor)
            self.setTextCursor(cursor)
            return
            
        # If we couldn't find any parentheses, try to find an arithmetic expression
        if not cursor.hasSelection():
            expr = self.find_arithmetic_expression(text, pos)
            if expr:
                start, end = expr
                cursor.setPosition(block_pos + start)
                cursor.setPosition(block_pos + end, QTextCursor.KeepAnchor)
                self.setTextCursor(cursor)

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

    def keyPressEvent(self, event):
        # Handle completion list navigation first
        if self.completion_list.isVisible():
            key = event.key()
            if key in (Qt.Key_Up, Qt.Key_Down, Qt.Key_Enter, Qt.Key_Return, Qt.Key_Tab, Qt.Key_Escape):
                if self.completion_list.handle_key_event(key):
                    event.accept()
                    return

        # Handle original key bindings
        ctrl = event.modifiers() & Qt.ControlModifier
        alt = event.modifiers() & Qt.AltModifier
        k = event.key()
        
        # Store scroll position before key handling for arrow keys
        old_scroll = self.verticalScrollBar().value()
        
        # Get the current text before modification
        cursor = self.textCursor()
        block = cursor.block()
        text_before = block.text()

        # Handle special keys and shortcuts
        if k == Qt.Key_C and ctrl and not alt:
            # Handle Ctrl+C to copy result or selection
            cursor = self.textCursor()
            if cursor.hasSelection():
                # If there's a selection, copy it normally
                text = cursor.selectedText().replace('\u2029', '\n').replace('\u2028', '\n')
                QApplication.clipboard().setText(text)
            else:
                # If no selection, copy the result from the current line
                line_number = cursor.blockNumber()
                if hasattr(self.parent, 'results'):
                    # First try to get the raw value
                    if hasattr(self.parent, 'raw_values') and line_number in self.parent.raw_values:
                        result_text = str(self.parent.raw_values[line_number])
                    else:
                        # Fall back to the displayed text
                        doc = self.parent.results.document()
                        if line_number < doc.blockCount():
                            result_block = doc.findBlockByNumber(line_number)
                            result_text = result_block.text()
                    
                    # Copy to clipboard
                    QApplication.clipboard().setText(result_text)
            return
            
        if k == Qt.Key_C and alt and not ctrl:
            # Handle Alt+C to copy formula
            cursor = self.textCursor()
            if cursor.hasSelection():
                # If there's a selection, copy it normally
                text = cursor.selectedText().replace('\u2029', '\n').replace('\u2028', '\n')
                QApplication.clipboard().setText(text)
            else:
                # If no selection, copy the formula from the current line
                cursor.movePosition(QTextCursor.StartOfLine)
                cursor.movePosition(QTextCursor.EndOfLine, QTextCursor.KeepAnchor)
                text = cursor.selectedText()
                QApplication.clipboard().setText(text)
            return

        # Handle Ctrl+Up/Down for selection
        if ctrl:
            if k == Qt.Key_Up:
                self.expand_selection_with_parens()
                event.accept()
                return
            elif k == Qt.Key_Down:
                self.select_entire_line()
                event.accept()
                return
            elif k == Qt.Key_Left:
                self.select_number_token(forward=False)
                event.accept()
                return
            elif k == Qt.Key_Right:
                self.select_number_token(forward=True)
                event.accept()
                return

        # Call parent implementation
        super().keyPressEvent(event)
        
        # Check if scroll position changed due to arrow keys or other navigation
        new_scroll = self.verticalScrollBar().value()
        if old_scroll != new_scroll and k in (Qt.Key_Up, Qt.Key_Down, Qt.Key_PageUp, Qt.Key_PageDown, Qt.Key_Home, Qt.Key_End):
            # Force scroll synchronization for navigation keys
            if hasattr(self.parent, '_sync_editor_to_results'):
                self.parent._sync_editor_to_results(new_scroll)

        # Get the current text after modification
        cursor = self.textCursor()
        block = cursor.block()
        text_after = block.text()

        # Handle auto-correction for cross-sheet references
        if (k in (Qt.Key_Space, Qt.Key_Return, Qt.Key_Enter, Qt.Key_Tab) or 
            (event.text() in '+-*/^().')):
            text = block.text()
            # Look for lowercase 's.' followed by any text and '.ln'
            matches = list(re.finditer(r'\bs\.(.*?)\.ln', text, re.IGNORECASE))
            if matches:
                # Create a new cursor for replacement
                replace_cursor = QTextCursor(block)
                for match in matches:
                    start_pos = match.start()
                    # Select and replace just the 's.' part
                    replace_cursor.setPosition(block.position() + start_pos)
                    replace_cursor.movePosition(QTextCursor.Right, QTextCursor.KeepAnchor, 2)
                    replace_cursor.insertText('S.')
                    # Also capitalize the '.ln' part
                    ln_pos = start_pos + len(match.group(1)) + 3  # Skip 's.' and the sheet name and the dot
                    replace_cursor.setPosition(block.position() + ln_pos)
                    replace_cursor.movePosition(QTextCursor.Right, QTextCursor.KeepAnchor, 2)
                    replace_cursor.insertText('LN')

        # Handle other auto-corrections and completions
        if event.text() and event.text().isprintable() and not event.text().isspace():
            current_word = self.get_word_under_cursor()
            if current_word:
                # Show completions for function names or currency patterns
                cursor = self.textCursor()
                line_text = cursor.block().text()
                cursor_pos = cursor.positionInBlock()
                
                # Check for currency conversion context
                has_currency_context = (
                    re.search(r'([\d.]+)\s+\w+\s+to\s+\w*$', line_text[:cursor_pos], re.IGNORECASE) or
                    re.search(r'([\d.]+)\s+\w+$', line_text[:cursor_pos], re.IGNORECASE)
                )
                
                if (any(func.lower().startswith(current_word.lower()) for func in self.base_completions) or 
                    has_currency_context):
                    self.show_completion_popup()
                else:
                    self.completion_list.hide()
            else:
                self.completion_list.hide()
        elif k in (Qt.Key_Backspace, Qt.Key_Delete):
            # Only show popup if there's still text to complete after deletion
            current_word = self.get_word_under_cursor()
            cursor = self.textCursor()
            line_text = cursor.block().text()
            cursor_pos = cursor.positionInBlock()
            
            # Check for currency conversion context
            has_currency_context = (
                re.search(r'([\d.]+)\s+\w+\s+to\s+\w*$', line_text[:cursor_pos], re.IGNORECASE) or
                re.search(r'([\d.]+)\s+\w+$', line_text[:cursor_pos], re.IGNORECASE)
            )
            
            if (current_word and 
                (any(func.lower().startswith(current_word.lower()) for func in self.base_completions) or
                 has_currency_context)):
                self.show_completion_popup()
            else:
                self.completion_list.hide()
        elif k in (Qt.Key_Up, Qt.Key_Down, Qt.Key_Left, Qt.Key_Right):
            # Hide completions during navigation
            self.completion_list.hide()

        # Check if we're potentially typing an LN reference and force evaluation
        # BUT exclude arrow keys to allow normal navigation through LN variables
        if (re.search(r'\bln\d+\b', text_after, re.IGNORECASE) and 
            k not in (Qt.Key_Left, Qt.Key_Right, Qt.Key_Up, Qt.Key_Down)):
            # Store the original cursor position
            original_cursor = self.textCursor()
            original_position = original_cursor.positionInBlock()
            
            # First capitalize ln to LN
            replace_cursor = QTextCursor(block)
            pos = 0
            position_offset = 0  # Track how much the cursor position should shift
            
            while True:
                match = re.search(r'\bln(\d+)\b', text_after[pos:], re.IGNORECASE)
                if not match:
                    break
                
                start_pos = pos + match.start()
                end_pos = pos + match.end()
                
                # Check if this replacement affects the cursor position
                if start_pos <= original_position:
                    # If the replacement is before or at the cursor, we need to adjust
                    if end_pos <= original_position:
                        # Replacement is completely before cursor - no change needed since LN is same length as ln
                        pass
                    else:
                        # Replacement overlaps with cursor position - move cursor to end of replacement
                        position_offset = (start_pos + 2 + len(match.group(1))) - original_position
                
                # Select and replace the text
                replace_cursor.setPosition(block.position() + start_pos)
                replace_cursor.setPosition(block.position() + end_pos, QTextCursor.KeepAnchor)
                replace_cursor.insertText(f"LN{match.group(1)}")
                
                # Update position and text for next iteration
                pos = start_pos + 2 + len(match.group(1))
                text_after = block.text()
            
            # Restore the original cursor position (adjusted for any replacements)
            if position_offset != 0:
                new_position = original_position + position_offset
            else:
                new_position = original_position
            
            # Make sure the new position is within bounds
            new_position = max(0, min(new_position, len(text_after)))
            
            # Set the cursor back to the original position
            original_cursor.setPosition(block.position() + new_position)
            self.setTextCursor(original_cursor)

        # Check for events that should trigger LN capitalization
        should_check_ln = (
            k in (Qt.Key_Space, Qt.Key_Return, Qt.Key_Enter, Qt.Key_Tab) or
            (event.text() in '+-*/^()')
        )

        if should_check_ln:
            # Look for lowercase 'ln' followed by numbers
            if re.search(r'\bln\d+\b', text_after, re.IGNORECASE):
                # Store the original cursor position
                original_cursor = self.textCursor()
                original_position = original_cursor.positionInBlock()
                
                # Create a new cursor for replacement
                replace_cursor = QTextCursor(block)
                
                # Replace all instances of 'ln' with 'LN' in this line
                pos = 0
                position_offset = 0  # Track how much the cursor position should shift
                
                while True:
                    match = re.search(r'\bln(\d+)\b', text_after[pos:], re.IGNORECASE)
                    if not match:
                        break
                    
                    start_pos = pos + match.start()
                    end_pos = pos + match.end()
                    
                    # Check if this replacement affects the cursor position
                    if start_pos <= original_position:
                        # If the replacement is before or at the cursor, we need to adjust
                        if end_pos <= original_position:
                            # Replacement is completely before cursor - no change needed since LN is same length as ln
                            pass
                        else:
                            # Replacement overlaps with cursor position - move cursor to end of replacement
                            position_offset = (start_pos + 2 + len(match.group(1))) - original_position
                    
                    # Select and replace the text
                    replace_cursor.setPosition(block.position() + start_pos)
                    replace_cursor.setPosition(block.position() + end_pos, QTextCursor.KeepAnchor)
                    replace_cursor.insertText(f"LN{match.group(1)}")
                    
                    # Update position and text for next iteration
                    pos = start_pos + 2 + len(match.group(1))
                    text_after = block.text()
                
                # Restore the original cursor position (adjusted for any replacements)
                if position_offset != 0:
                    new_position = original_position + position_offset
                else:
                    new_position = original_position
                
                # Make sure the new position is within bounds
                new_position = max(0, min(new_position, len(text_after)))
                
                # Set the cursor back to the original position
                original_cursor.setPosition(block.position() + new_position)
                self.setTextCursor(original_cursor)

    def wheelEvent(self, event):
        if event.modifiers() & Qt.ControlModifier:
            delta = event.angleDelta().y()
            if delta > 0:
                self.change_font_size(1)
            elif delta < 0:
                self.change_font_size(-1)
            event.accept()
        else:
            super().wheelEvent(event)

    def mousePressEvent(self, event):
        if event.button() == Qt.MiddleButton and event.modifiers() & Qt.ControlModifier:
            self.reset_font_size()
            event.accept()
        else:
            # Track when a potential selection operation starts
            if event.button() == Qt.LeftButton:
                self._selection_in_progress = True
            super().mousePressEvent(event)

    def mouseReleaseEvent(self, event):
        # Clear selection tracking when mouse is released
        if event.button() == Qt.LeftButton:
            self._selection_in_progress = False
        super().mouseReleaseEvent(event)

    def change_font_size(self, delta):
        new_size = max(6, min(72, self.current_font_size + delta))
        if new_size != self.current_font_size:
            self.current_font_size = new_size
            self.parent.settings.setValue('font_size', new_size)
            self.update_fonts()

    def reset_font_size(self):
        self.current_font_size = self.default_font_size
        self.parent.settings.setValue('font_size', self.default_font_size)
        self.update_fonts()

    def update_fonts(self):
        # Update editor font
        font = QFont("Courier New", self.current_font_size, QFont.Bold)
        self.setFont(font)
        # Update line number area
        self.updateLineNumberAreaWidth(0)
        # Update results panel font
        self.parent.results.setFont(font)
        # Force refresh
        self.parent.evaluate()

    def show_completion_popup(self):
        text_cursor = self.textCursor()
        current_word = self.get_word_under_cursor()
        
        # Don't show completion for empty strings or numbers
        if not current_word or current_word.isdigit():
            self.completion_list.hide()
            return

        # Get completions
        completions = self.get_completions(current_word)
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
        height = min(self.completion_list.count() * 30, 300)  # 30 pixels per item, max 300px height
        
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

    def paintEvent(self, event):
        # Draw the base text edit content
        super().paintEvent(event)
        
        # Draw separator lines
        if self.separator_lines:
            painter = QPainter(self.viewport())
            painter.setPen(QPen(QColor(80, 80, 80), 1, Qt.DashLine))  # Light grey, dashed line
            
            # Get the full width including the scrolled area
            doc_width = max(self.document().size().width(), self.viewport().width())
            doc_width += self.horizontalScrollBar().value()  # Add scrolled amount
            
            # Draw lines above and below function lines
            for line_num in self.separator_lines:
                block = self.document().findBlockByLineNumber(line_num)
                if block.isValid():
                    # Get the geometry of the line
                    rect = self.blockBoundingGeometry(block).translated(self.contentOffset())
                    # Draw the line above
                    painter.drawLine(0, int(rect.top()) - 2, doc_width, int(rect.top()) - 2)
                    # Draw the line below
                    painter.drawLine(0, int(rect.bottom()) + 2, doc_width, int(rect.bottom()) + 2)
                    
            painter.end()

class Worksheet(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        layout = QVBoxLayout(self)
        self.splitter = QSplitter(Qt.Horizontal)
        self.settings = QSettings('OpenAI', 'SmartCalc')
        
        # Create results widget first - now using QPlainTextEdit for perfect alignment
        self.results = QPlainTextEdit()
        self.results.setReadOnly(True)
        font_size = self.settings.value('font_size', 14, type=int)
        self.results.setFont(QFont("Courier New", font_size, QFont.Bold))
        self.results.setStyleSheet("""
            QPlainTextEdit {
                background-color: #2c2c2e; 
                color: white;
                border: none;
                padding: 0px;
                margin: 0px;
                line-height: 1.2em;
            }
        """)
        self.results.setLineWrapMode(QPlainTextEdit.NoWrap)
        self.results.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOn)
        
        # Set document margins to match editor exactly
        results_doc = self.results.document()
        results_doc.setDocumentMargin(0)
        
        # Set text margins to zero
        self.results.setViewportMargins(0, 0, 0, 0)
        self.results.setContentsMargins(0, 0, 0, 0)
        
        # Add storage for raw values (for copying unformatted numbers)
        self.raw_values = {}  # line_number -> raw_value
        
        # Add syntax highlighter for error coloring
        self.results_highlighter = ResultsHighlighter(self.results.document())
        
        # Then create editor
        self.editor = FormulaEditor(self)
        
        # Add flag to prevent infinite recursion during scroll synchronization
        self._syncing_scroll = False
        
        # Connect scrollbars for synchronization
        self.editor.verticalScrollBar().valueChanged.connect(self._sync_editor_to_results)
        self.results.verticalScrollBar().valueChanged.connect(self._sync_results_to_editor)
        
        # Add widgets to splitter
        self.splitter.addWidget(self.editor)
        self.splitter.addWidget(self.results)
        self.splitter.setSizes([600, 200])
        layout.addWidget(self.splitter)
        
        # Setup evaluation timer
        self.timer = QTimer(self)
        self.timer.setInterval(300)
        self.timer.setSingleShot(True)
        self.timer.timeout.connect(self.evaluate_and_highlight)
        
        # Connect text changes to evaluation
        self.editor.textChanged.connect(self.timer.start)
        
        # Initial evaluation
        QTimer.singleShot(0, self.evaluate_and_highlight)

    def _sync_editor_to_results(self, value):
        """Sync results scrollbar when editor scrollbar changes"""
        if not self._syncing_scroll:
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

    def _sync_results_to_editor(self, value):
        """Sync editor scrollbar when results scrollbar changes"""
        if not self._syncing_scroll:
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

    def evaluate_and_highlight(self):
        """Evaluate formulas and ensure highlighting is updated"""
        self.evaluate()
        self.editor.highlightCurrentLine()

    def on_text_changed(self):
        """Only start evaluation timer if text actually changed"""
        if not hasattr(self.editor, '_cursor_triggered_eval'):
            self.timer.start()
        else:
            delattr(self.editor, '_cursor_triggered_eval')

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
                    if abs_val >= 1e6:
                        # Very large numbers - use scientific notation or limited precision
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

    def evaluate(self):
        """Evaluate formulas and update results"""
        # Store current cursor and scroll positions
        cursor = self.editor.textCursor()
        editor_scroll = self.editor.verticalScrollBar().value()
        results_scroll = self.results.verticalScrollBar().value()
        
        # Check if this is a cursor-triggered evaluation
        is_cursor_triggered = hasattr(self.editor, '_cursor_triggered_eval')
        if is_cursor_triggered:
            delattr(self.editor, '_cursor_triggered_eval')
        
        # Ensure line IDs are properly assigned
        self.editor.reassign_line_ids()
        
        lines = self.editor.toPlainText().split("\n")
        vals = [None]*len(lines)
        out = []
        self.editor.ln_value_map = {}
        id_map = {}
        doc = self.editor.document()
        
        # Update separator lines
        self.editor.update_separator_lines()
        
        # Build id_map and initialize ln_value_map
        for i in range(doc.blockCount()):
            blk = doc.findBlockByNumber(i)
            d = blk.userData()
            if isinstance(d, LineData):
                id_map[d.id] = i
                # Initialize with None to ensure the ID exists in the map
                self.editor.ln_value_map[d.id] = None

        def truncate(value, decimals=2):
            """Rounds a number to specified decimal places"""
            if isinstance(value, str):
                # If it's a string expression, evaluate it first
                try:
                    value = eval(value, {"truncate": truncate, **GLOBALS}, {})
                except:
                    return value
            if isinstance(value, dict) and 'value' in value:
                # Handle unit conversion results
                return {'value': round(value['value'] * (10 ** decimals)) / (10 ** decimals),
                       'unit': value['unit']}
            if not isinstance(value, (int, float)):
                return value
            factor = 10 ** decimals
            return round(value * factor) / factor

        def preprocess_expression(expr):
            """Pre-process expression to handle padded numbers and other special cases"""
            # Debug: Print original expression if it contains TC
            if 'TC(' in expr:
                print(f"DEBUG: Original TC expression: {expr}")
            
            # Handle timecode arithmetic first (BEFORE comma removal to preserve function arguments)
            tc_match = re.match(r'TC\((.*?)\)', expr)
            if tc_match:
                tc_args = tc_match.group(1)
                print(f"DEBUG: TC args extracted: '{tc_args}'")
                
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
                
                print(f"DEBUG: TC parts after splitting: {parts}")
                
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
                
                print(f"DEBUG: TC processed parts: {processed_parts}")
                
                # Reconstruct the TC call
                expr = f"TC({','.join(processed_parts)})"
                print(f"DEBUG: TC reconstructed expression: {expr}")
            
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
            def remove_thousands_commas(match):
                number_str = match.group(0)
                # Remove all commas from the number
                return number_str.replace(',', '')
            
            # Pattern to match numbers with commas like 1,234 or 1,234.56
            # Use negative lookbehind to avoid matching inside function calls
            # This pattern avoids matching numbers that come after an opening parenthesis
            comma_number_pattern = r'(?<!\()\b\d{1,3}(?:,\d{3})*(?:\.\d+)?\b(?![^()]*\))'
            expr = re.sub(comma_number_pattern, remove_thousands_commas, expr)
            
            # Replace numbers with leading zeros outside of timecodes and quoted strings
            def repl_num(m):
                # Don't replace if it's part of a timecode
                if re.match(r'\d{1,2}[:.]\d{1,2}[:.]\d{1,2}[:.]\d{1,2}', m.string[max(0, m.start()-8):m.end()+8]):
                    return m.group(0)
                # Don't replace if it's inside quotes
                before_match = m.string[:m.start()]
                after_match = m.string[m.end():]
                quote_count_before = before_match.count('"') - before_match.count('\\"')
                if quote_count_before % 2 == 1:  # We're inside quotes
                    return m.group(0)
                return str(int(m.group(1)))
            
            expr = re.sub(r'\b0+(\d+)\b', repl_num, expr)
            
            # Debug: Print final expression if it contains TC
            if 'TC(' in expr:
                print(f"DEBUG: Final TC expression: {expr}")
            
            return expr

        def handle_special_commands(expr, idx):
            """Handle special commands like sum() and mean()"""
            # Extract range or list from parentheses
            match = re.match(r'(\w+)\((.*?)\)', expr.strip())
            if not match:
                return None
            
            cmd_type, args = match.groups()
            cmd_type = cmd_type.lower()  # Make case-insensitive
            
            # Check if this is a special command
            if cmd_type not in ('sum', 'mean', 'median', 'mode', 'min', 'max', 'range', 'count', 
                              'product', 'variance', 'stdev', 'std', 'geomean', 'harmmean', 
                              'sumsq', 'perc5', 'perc95'):
                return None
            
            # Function to get numbers from a range of lines
            def get_numbers_from_range(start_end):
                numbers = []
                try:
                    if '-' in start_end and ',' not in start_end:
                        start, end = map(int, start_end.split('-'))
                        for i in range(start-1, end):
                            if i < len(vals) and vals[i] is not None and isinstance(vals[i], (int, float)):
                                numbers.append(float(vals[i]))
                    else:
                        for arg in start_end.split(','):
                            line_num = int(arg.strip()) - 1
                            if line_num < len(vals) and vals[line_num] is not None and isinstance(vals[line_num], (int, float)):
                                numbers.append(float(vals[line_num]))
                except:
                    pass
                return numbers

            # If no arguments provided, use all lines above
            if not args.strip():
                numbers = []
                for i in range(idx):
                    if vals[i] is not None and isinstance(vals[i], (int, float)):
                        numbers.append(float(vals[i]))
            else:
                numbers = get_numbers_from_range(args)
            
            if not numbers:
                return 0 if cmd_type == 'sum' else None

            # Apply the appropriate operation
            try:
                if cmd_type == 'sum':
                    return sum(numbers)
                elif cmd_type in ('mean', 'average'):
                    return sum(numbers) / len(numbers)
                elif cmd_type == 'median':
                    return statistics.median(numbers)
                elif cmd_type == 'mode':
                    try:
                        return statistics.mode(numbers)
                    except statistics.StatisticsError:
                        return None  # No unique mode
                elif cmd_type == 'min':
                    return min(numbers)
                elif cmd_type == 'max':
                    return max(numbers)
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

        def handle_unit_conversion(expr):
            """Handle unit conversion expressions like '1 mile to km'"""
            match = re.match(r'^([\d.]+)\s+(\w+)\s+to\s+(\w+)$', expr.strip())
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
                out.append("&nbsp;")
                continue
            
            if s.startswith(":::"):  # Comment line
                vals[idx] = None
                blk = doc.findBlockByNumber(idx)
                data = blk.userData()
                if isinstance(data, LineData):
                    self.editor.ln_value_map[data.id] = vals[idx]
                out.append("&nbsp;")
                continue

            # Try special cases first
            try:
                # Get the current block and its ID
                blk = doc.findBlockByNumber(idx)
                data = blk.userData()
                current_id = data.id if isinstance(data, LineData) else None

                # Pre-process the expression to handle padded numbers
                s = preprocess_expression(s)

                # Check for date expressions first
                if re.search(r'[dD]\.?(?:\d|[A-Za-z]+\s)', s):
                    # Auto-correct all lowercase 'd' to uppercase 'D'
                    line_cursor = QTextCursor(blk)
                    text = line_cursor.block().text()
                    pos = 0
                    
                    # Find all lowercase 'd' date prefixes and replace them
                    while True:
                        match = re.search(r'\bd\.?(?=(?:\d|[A-Za-z]+\s))', text[pos:])
                        if not match:
                            break
                            
                        start_pos = pos + match.start()
                        # Create a cursor at the 'd' position
                        line_cursor.setPosition(line_cursor.block().position() + start_pos)
                        line_cursor.movePosition(QTextCursor.Right, QTextCursor.KeepAnchor, 1)
                        # Replace with uppercase 'D'
                        line_cursor.insertText('D')
                        
                        # Update text and position for next iteration
                        text = line_cursor.block().text()
                        pos = start_pos + 1
                    
                    # Update the line text for evaluation
                    s = text.strip()
                    
                    date_result = handle_date_arithmetic(s)
                    if date_result is not None:
                        vals[idx] = date_result
                        if current_id:
                            self.editor.ln_value_map[current_id] = vals[idx]
                        out.append(self.format_number_for_display(date_result, idx))
                        continue

                # Check for unit conversion
                unit_result = handle_unit_conversion(s)
                if unit_result is not None:
                    vals[idx] = unit_result
                    if current_id:
                        self.editor.ln_value_map[current_id] = vals[idx]
                    out.append(self.format_number_for_display(unit_result, idx))
                    continue

                # Check for currency conversion
                currency_result = handle_currency_conversion(s)
                if currency_result is not None:
                    vals[idx] = currency_result
                    if current_id:
                        self.editor.ln_value_map[current_id] = vals[idx]
                    out.append(self.format_number_for_display(currency_result, idx))
                    continue

                # Check for truncate function call (both truncate and TR)
                trunc_match = re.match(r'(?:truncate|TR)\((.*?),(.*?)\)', s)
                if trunc_match:
                    # First evaluate the expression
                    expr = self.editor.process_ln_refs(trunc_match.group(1).strip())
                    decimals = int(eval(trunc_match.group(2).strip(), {"truncate": truncate, **GLOBALS}, {}))
                    
                    # Try unit conversion first
                    unit_result = handle_unit_conversion(expr)
                    if unit_result is not None:
                        v = truncate(unit_result, decimals)
                    else:
                        # Try currency conversion
                        currency_result = handle_currency_conversion(expr)
                        if currency_result is not None:
                            v = truncate(currency_result, decimals)
                        else:
                            # If not a unit or currency conversion, evaluate as regular expression
                            val = eval(expr, {"truncate": truncate, **GLOBALS}, {})
                            v = truncate(val, decimals)
                        
                    vals[idx] = v
                    if current_id:
                        self.editor.ln_value_map[current_id] = vals[idx]
                    out.append(self.format_number_for_display(v, idx))
                    continue

                # Try special commands
                cmd_result = handle_special_commands(s, idx)
                if cmd_result is not None:
                    vals[idx] = cmd_result
                    if current_id:
                        self.editor.ln_value_map[current_id] = vals[idx]
                    out.append(self.format_number_for_display(cmd_result, idx))
                    continue

                # Process LN references if present
                if re.search(r"\b(?:s\.|S\.)?(?:ln|LN)\d+\b", s, re.IGNORECASE):
                    s = self.editor.process_ln_refs(s)
                    print(f"Line {idx + 1} after processing refs: {s}")  # Debug print

                # Try to evaluate the expression with math functions
                v = eval(s, {"truncate": truncate, "mean": statistics.mean, **GLOBALS}, {})
                vals[idx] = v
                if current_id:
                    self.editor.ln_value_map[current_id] = vals[idx]
                    print(f"Stored value {v} for line ID {current_id}")  # Debug print
                
                # Format the output
                out.append(self.format_number_for_display(v, idx))
            except TimecodeError as e:
                # Handle TimecodeError specifically to show the actual error message
                print(f"Timecode error on line {idx + 1}: {str(e)}")  # Debug print
                vals[idx] = None
                if current_id:
                    self.editor.ln_value_map[current_id] = None
                out.append(f'TC ERROR: {str(e)}')
            except Exception as e:
                print(f"Error evaluating line {idx + 1}: {str(e)}")  # Debug print
                vals[idx] = None
                if current_id:
                    self.editor.ln_value_map[current_id] = None
                out.append('ERROR!')

        # Update results with plain text (no HTML needed since we're using QPlainTextEdit)
        text_content = '\n'.join(out)
        self.results.setPlainText(text_content)
        
        # Restore cursor and synchronized scroll position
        self.editor.setTextCursor(cursor)
        # Only set one scrollbar - the synchronization will handle the other
        self._syncing_scroll = True  # Temporarily disable sync to avoid double-setting
        self.editor.verticalScrollBar().setValue(editor_scroll)
        self._syncing_scroll = False
        
        # Force a sync after content update to ensure both are aligned
        QTimer.singleShot(10, lambda: self._force_sync_from_editor())
        
        # Force highlight update if this was cursor-triggered
        if is_cursor_triggered:
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

class Calculator(QWidget):
    def __init__(self):
        super().__init__()
        # Set window flags to ensure it appears on top initially
        self.setWindowFlags(self.windowFlags() | Qt.WindowStaysOnTopHint)
        
        # Set window icon
        icon_path = os.path.join(os.path.dirname(sys.argv[0]), "calcforge.ico")
        if os.path.exists(icon_path):
            self.setWindowIcon(QIcon(icon_path))
        
        self.settings = QSettings('OpenAI','SmartCalc')
        if self.settings.contains('geometry'):
            self.restoreGeometry(self.settings.value('geometry'))
        self.splitter_state = self.settings.value('splitterState')
        self.setWindowTitle("CalcForge v3.1")
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
        main.addWidget(self.tabs)
        
        # Load saved worksheets or create new one
        wf = Path(os.path.dirname(sys.argv[0]))/"worksheets.json"
        if wf.exists():
            try:
                data = json.loads(wf.read_text())
                self.tabs.clear()
                for name, content in data.items():
                    ws = Worksheet(self)
                    self.tabs.addTab(ws, name)
                    ws.editor.setPlainText(content)
                    if self.splitter_state:
                        ws.splitter.restoreState(self.splitter_state)
                # Position cursor at end of first sheet
                if self.tabs.count() > 0:
                    self.position_cursor_at_end(self.tabs.widget(0).editor)
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

    def add_tab(self):
        ws=Worksheet()
        idx=self.tabs.addTab(ws,f"Sheet {self.tabs.count()+1}")
        self.tabs.setCurrentIndex(idx)
        if self.splitter_state:
            ws.splitter.restoreState(self.splitter_state)
        # Position cursor at end for new tabs
        self.position_cursor_at_end(ws.editor)

    def close_tab(self,idx):
        if self.tabs.count()>1: self.tabs.removeTab(idx)

    def rename_tab(self,idx):
        if idx>=0:
            text,ok=QInputDialog.getText(self,"Rename Sheet","New name:")
            if ok and text: self.tabs.setTabText(idx,text)

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
        from PySide6.QtWidgets import QDialog, QVBoxLayout, QTextEdit, QPushButton, QHBoxLayout
        from PySide6.QtCore import Qt
        from PySide6.QtGui import QIcon
        
        dialog = QDialog(self)
        dialog.setWindowTitle("CalcForge Help")
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
            f"<h1 style='text-align: center; color: #4da6ff; margin-bottom: 20px;'>{icon_html}CalcForge v3.1 - Complete Reference Guide</h1>"
            
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
            "• Example: <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>truncate(pi, 3) → 3.142</code><br><br>"
            
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
            "<h3 style='color: #6fcf97; margin-top: 0;'>📈 Statistical Functions</h3>"
            "• <strong style='color: #ff9999;'>Basic Statistics:</strong><br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>sum(1-5), mean(1-5), median(1-5), mode(1-5)</code><br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>min(1-5), max(1-5), range(1-5), count(1-5)</code><br><br>"
            
            "• <strong style='color: #ff9999;'>Advanced Statistics:</strong><br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>variance(1-5), stdev(1-5), geomean(1-5)</code><br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>harmmean(1-5), product(1-5), sumsq(1-5)</code><br>"
            "  <code style='background-color: #333; color: #ffd700; padding: 2px 4px; border-radius: 3px;'>perc5(1-5), perc95(1-5)</code><br><br>"
            
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
            "• Auto-completion for functions<br>"
            "• Live result updates<br>"
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
        """Disabled to prevent cursor synchronization issues with cross-sheet highlighting"""
        pass

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

class ResultsHighlighter(QSyntaxHighlighter):
    def __init__(self, document):
        super().__init__(document)
        self.error_format = self._fmt("#FF5C5C", bold=True)
        
    def _fmt(self, color, bold=False):
        fmt = QTextCharFormat()
        fmt.setForeground(QColor(color))
        if bold:
            fmt.setFontWeight(QFont.Bold)
        
    def highlightBlock(self, text):
        # Highlight error text
        if "ERROR" in text or "TC ERROR" in text:
            self.setFormat(0, len(text), self.error_format)

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
            ctypes.windll.shell32.SetCurrentProcessExplicitAppUserModelID("CalcForge.2.0")
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
