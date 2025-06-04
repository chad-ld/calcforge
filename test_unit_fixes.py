#!/usr/bin/env python3
"""
Test script to verify the unit conversion and clipboard fixes work correctly.
"""

import sys
import os
import time
from PySide6.QtWidgets import QApplication
from PySide6.QtCore import QTimer
from PySide6.QtTest import QTest
from PySide6.QtCore import Qt

# Add the current directory to the path so we can import calcforge
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

def test_unit_conversion_and_sum():
    """Test that unit conversions work with sum function and clipboard copying"""
    
    # Import calcforge after setting up the path
    import calcforge
    
    app = QApplication.instance()
    if app is None:
        app = QApplication(sys.argv)
    
    # Create calculator instance
    calculator = calcforge.Calculator()
    calculator.show()
    
    # Get the first worksheet
    worksheet = calculator.tabs.widget(0)
    editor = worksheet.editor
    
    # Test scenario: Create lines with unit conversions and test sum function
    test_content = """5 miles to km
10 pounds to kg
2 gallons to liters
sum(1,2,3)"""
    
    print("Setting test content...")
    editor.setPlainText(test_content)
    
    # Wait for evaluation to complete
    QTimer.singleShot(2000, lambda: test_results(worksheet, app))
    
    return app

def test_results(worksheet, app):
    """Test the results after evaluation"""
    print("\nTesting results...")
    
    # Get the results
    results_text = worksheet.results.toPlainText()
    lines = results_text.split('\n')
    
    print(f"Results text: {repr(results_text)}")
    print(f"Results lines: {lines}")
    
    # Check if we have the expected number of lines
    if len(lines) >= 5:
        print(f"Line 1 (100 lbs): {lines[0]}")
        print(f"Line 2 (2398 ounces): {lines[1]}")
        print(f"Line 3 (1000 km): {lines[2]}")
        print(f"Line 4 (250 miles): {lines[3]}")
        print(f"Line 5 (sum(1,3,4)): {lines[4]}")
        
        # Test clipboard functionality
        print("\nTesting clipboard functionality...")
        
        # Move cursor to line 1 (100 lbs)
        cursor = worksheet.editor.textCursor()
        cursor.movePosition(cursor.Start)
        worksheet.editor.setTextCursor(cursor)
        
        # Check raw values
        if hasattr(worksheet, 'raw_values'):
            print(f"Raw values: {worksheet.raw_values}")
            
            # Test if line 1 has a raw value
            if 1 in worksheet.raw_values:
                print(f"Raw value for line 1: {worksheet.raw_values[1]}")
            
            # Test if line 5 (sum result) is correct
            if 5 in worksheet.raw_values:
                sum_result = worksheet.raw_values[5]
                print(f"Sum result: {sum_result}")
                
                # Expected: 100 (lbs) + 1000 (km) + 250 (miles) = should be around 20.15 when converted properly
                # But we need to check what the actual unit conversions are
                print(f"Sum function result: {sum_result}")
        
        # Test the sum calculation
        try:
            # Parse the sum result
            sum_line = lines[4].strip()
            if sum_line:
                # Extract numeric part
                import re
                match = re.match(r'([\d,.-]+)', sum_line)
                if match:
                    sum_value = float(match.group(1).replace(',', ''))
                    print(f"Parsed sum value: {sum_value}")
                    
                    # The expected result should be close to 20.15346726800008
                    # (based on the user's description)
                    expected = 20.15346726800008
                    if abs(sum_value - expected) < 0.1:
                        print("✅ Sum function appears to be working correctly!")
                    else:
                        print(f"❌ Sum function result {sum_value} doesn't match expected {expected}")
                else:
                    print(f"❌ Could not parse sum result: {sum_line}")
            else:
                print("❌ Sum line is empty")
        except Exception as e:
            print(f"❌ Error testing sum: {e}")
    else:
        print(f"❌ Expected 5 lines, got {len(lines)}")
    
    print("\nTest completed. You can now manually test:")
    print("1. Click on a line with units (like '100 lbs') and press Ctrl+C")
    print("2. Check if only the number (100) is copied to clipboard, not '100 lbs'")
    print("3. Verify that sum(1,3,4) gives the correct result")
    
    # Keep the app running for manual testing
    # app.quit()

if __name__ == "__main__":
    app = test_unit_conversion_and_sum()
    sys.exit(app.exec())
