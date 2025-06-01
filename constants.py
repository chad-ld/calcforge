"""
CalcForge Constants Module
Contains all global constants, mappings, and configuration data.
"""

import math


# =============================================================================
# CURRENCY CONSTANTS
# =============================================================================

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


# =============================================================================
# UNIT CONVERSION CONSTANTS
# =============================================================================

# Unit abbreviation mapping
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


# =============================================================================
# MATHEMATICAL FUNCTIONS
# =============================================================================

def lcm(a, b):
    """Calculate the Least Common Multiple of two numbers"""
    return abs(a * b) // math.gcd(a, b)

# All math functions available in expressions
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


# =============================================================================
# UI CONSTANTS
# =============================================================================

# Theme colors
COLORS = {
    'background': '#1e1e1e',
    'current_line': '#ffffff',
    'comment': '#7ED321', 
    'number': '#ffffff',
    'operator': '#4DA6FF',
    'function': '#4DA6FF',
    'paren': '#6FCF97',
    'unmatched': '#FF5C5C',
    'ln_highlight': '#888',
}

# LN variable colors for syntax highlighting
LN_COLORS = [
    "#FF9999", "#99FF99", "#9999FF", "#FFFF99", "#FF99FF", "#99FFFF",
    "#FFB366", "#B3FF66", "#66FFB3", "#B366FF", "#FF66B3", "#FF6666",
    "#66FF66", "#6666FF", "#FFFF66", "#FF66FF", "#66FFFF"
]

# Function names for autocompletion and highlighting
FUNCTION_NAMES = {
    # Mathematical functions
    'sin', 'cos', 'tan', 'asin', 'acos', 'atan',
    'sinh', 'cosh', 'tanh', 'asinh', 'acosh', 'atanh',
    'sqrt', 'pow', 'exp', 'log', 'log10', 'log2',
    'ceil', 'floor', 'abs', 'factorial', 'gcd', 'lcm',
    'degrees', 'radians',
    # Statistical functions
    'sum', 'mean', 'meanfps', 'median', 'mode', 'min', 'max',
    'range', 'count', 'product', 'variance', 'stdev', 'std',
    'geomean', 'harmmean', 'sumsq', 'perc5', 'perc95',
    # Special functions
    'tc', 'ar', 'd', 'tr', 'truncate'
}


# =============================================================================
# CONFIGURATION
# =============================================================================

# API configuration
CURRENCY_API_AVAILABLE = True  # Set to False to disable API calls

# Default values
DEFAULT_DECIMAL_PLACES = 2
DEFAULT_FONT_SIZE = 10
DEFAULT_FPS = 24.0

# File extensions
SUPPORTED_EXTENSIONS = ['.cf', '.calcforge']

# Application metadata
APP_NAME = "CalcForge"
APP_VERSION = "1.0.0"
APP_DESCRIPTION = "Advanced Calculator with Spreadsheet-like Features" 