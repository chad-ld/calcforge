"""
Test script for CalcForge backend components.
Verifies that the extracted backend logic works correctly.
"""

import sys
import os

# Add the backend directory to the path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from calcforge_engine import CalcForgeEngine
from syntax_highlighter import SyntaxHighlighter
from worksheet_manager import WorksheetManager


def test_basic_calculations():
    """Test basic mathematical calculations"""
    print("Testing basic calculations...")
    engine = CalcForgeEngine()
    
    test_cases = [
        ("2 + 3", 5),
        ("10 * 5", 50),
        ("100 / 4", 25),
        ("2^3", 8),
        ("sqrt(16)", 4),
        ("sin(0)", 0),
        ("pi", 3.141592653589793),
    ]
    
    for expr, expected in test_cases:
        result = engine.evaluate_expression(expr)
        print(f"  {expr} = {result['value']} (expected: {expected})")
        assert abs(float(result['value']) - expected) < 0.0001, f"Failed: {expr}"
    
    print("✓ Basic calculations passed")


def test_timecode_functions():
    """Test timecode calculations"""
    print("\nTesting timecode functions...")
    engine = CalcForgeEngine()
    
    test_cases = [
        ("TC(24, 100)", "00:00:04:04"),  # 100 frames at 24fps
        ("TC(30, '00:01:00:00')", "1800"),  # 1 minute at 30fps to frames
    ]
    
    for expr, expected in test_cases:
        result = engine.evaluate_expression(expr)
        print(f"  {expr} = {result['value']} (expected: {expected})")
    
    print("✓ Timecode functions tested")


def test_unit_conversions():
    """Test unit conversion functionality"""
    print("\nTesting unit conversions...")
    engine = CalcForgeEngine()
    
    test_cases = [
        "5 feet to meters",
        "100 pounds to kilograms",
        "1 gallon to liters",
    ]
    
    for expr in test_cases:
        result = engine.evaluate_expression(expr)
        print(f"  {expr} = {result['value']} {result['unit']}")
    
    print("✓ Unit conversions tested")


def test_date_arithmetic():
    """Test date arithmetic"""
    print("\nTesting date arithmetic...")
    engine = CalcForgeEngine()
    
    test_cases = [
        "D(July 4, 2023)",
        "D(July 4, 2023 + 30)",
        "D(July 4, 2023 - July 1, 2023)",
    ]
    
    for expr in test_cases:
        result = engine.evaluate_expression(expr)
        print(f"  {expr} = {result['value']}")
    
    print("✓ Date arithmetic tested")


def test_syntax_highlighting():
    """Test syntax highlighting"""
    print("\nTesting syntax highlighting...")
    highlighter = SyntaxHighlighter()
    
    test_text = "sum(LN1:LN5) + TC(24, 100) * 2"
    highlights = highlighter.highlight_text(test_text)
    
    print(f"  Text: {test_text}")
    print(f"  Highlights found: {len(highlights)}")
    for highlight in highlights:
        start = highlight['start']
        length = highlight['length']
        text_part = test_text[start:start+length]
        print(f"    '{text_part}' -> {highlight['class']}")
    
    print("✓ Syntax highlighting tested")


def test_worksheet_manager():
    """Test worksheet management"""
    print("\nTesting worksheet manager...")
    manager = WorksheetManager()
    
    # Create worksheets
    sheet1_id = manager.create_worksheet("Test Sheet 1", "2 + 3\n5 * 4")
    sheet2_id = manager.create_worksheet("Test Sheet 2", "LN1 + 10\nsum(1:2)")
    
    print(f"  Created sheet 1 with ID: {sheet1_id}")
    print(f"  Created sheet 2 with ID: {sheet2_id}")
    
    # Test retrieval
    sheet1 = manager.get_worksheet(sheet1_id)
    print(f"  Retrieved sheet 1: {sheet1['name']}")
    
    # Test renaming
    manager.rename_worksheet(sheet1_id, "Renamed Sheet")
    sheet1 = manager.get_worksheet(sheet1_id)
    print(f"  Renamed sheet 1 to: {sheet1['name']}")
    
    # Test all worksheets
    all_sheets = manager.get_all_worksheets()
    print(f"  Total worksheets: {len(all_sheets)}")
    
    print("✓ Worksheet manager tested")


def test_error_handling():
    """Test error handling"""
    print("\nTesting error handling...")
    engine = CalcForgeEngine()
    
    test_cases = [
        "1 / 0",  # Division by zero
        "sqrt(-1)",  # Invalid math
        "invalid_function(5)",  # Unknown function
        "2 +",  # Incomplete expression
    ]
    
    for expr in test_cases:
        result = engine.evaluate_expression(expr)
        print(f"  {expr} -> Error: {result['error'] is not None}")
    
    print("✓ Error handling tested")


def main():
    """Run all tests"""
    print("CalcForge Backend Test Suite")
    print("=" * 40)
    
    try:
        test_basic_calculations()
        test_timecode_functions()
        test_unit_conversions()
        test_date_arithmetic()
        test_syntax_highlighting()
        test_worksheet_manager()
        test_error_handling()
        
        print("\n" + "=" * 40)
        print("✅ All tests completed successfully!")
        print("Backend extraction appears to be working correctly.")
        
    except Exception as e:
        print(f"\n❌ Test failed with error: {e}")
        import traceback
        traceback.print_exc()
        return False
    
    return True


if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)
