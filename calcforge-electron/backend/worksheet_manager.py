"""
CalcForge Worksheet Manager - Backend Logic for Tab/Worksheet Management
Handles worksheet data, cross-sheet references, and file operations.
"""

import json
import os
from typing import Dict, List, Optional, Any
from pathlib import Path


class WorksheetManager:
    """
    Manages worksheet data, file operations, and cross-sheet references.
    """
    
    def __init__(self):
        self.worksheets: Dict[int, Dict[str, Any]] = {}
        self.next_sheet_id = 1
        self.current_sheet_id = 0
        
    def create_worksheet(self, name: str = None, content: str = "") -> int:
        """
        Create a new worksheet.
        
        Args:
            name (str): Name of the worksheet
            content (str): Initial content
            
        Returns:
            int: ID of the created worksheet
        """
        sheet_id = self.next_sheet_id
        self.next_sheet_id += 1
        
        if name is None:
            name = f"Sheet {sheet_id}"
        
        self.worksheets[sheet_id] = {
            "id": sheet_id,
            "name": name,
            "content": content,
            "created_at": None,
            "modified_at": None
        }
        
        return sheet_id
    
    def delete_worksheet(self, sheet_id: int) -> bool:
        """
        Delete a worksheet.
        
        Args:
            sheet_id (int): ID of the worksheet to delete
            
        Returns:
            bool: True if deleted successfully
        """
        if sheet_id in self.worksheets:
            del self.worksheets[sheet_id]
            return True
        return False
    
    def rename_worksheet(self, sheet_id: int, new_name: str) -> bool:
        """
        Rename a worksheet.
        
        Args:
            sheet_id (int): ID of the worksheet
            new_name (str): New name for the worksheet
            
        Returns:
            bool: True if renamed successfully
        """
        if sheet_id in self.worksheets:
            self.worksheets[sheet_id]["name"] = new_name
            return True
        return False
    
    def update_worksheet_content(self, sheet_id: int, content: str) -> bool:
        """
        Update worksheet content.
        
        Args:
            sheet_id (int): ID of the worksheet
            content (str): New content
            
        Returns:
            bool: True if updated successfully
        """
        if sheet_id in self.worksheets:
            self.worksheets[sheet_id]["content"] = content
            return True
        return False
    
    def get_worksheet(self, sheet_id: int) -> Optional[Dict[str, Any]]:
        """
        Get worksheet data.
        
        Args:
            sheet_id (int): ID of the worksheet
            
        Returns:
            dict: Worksheet data or None if not found
        """
        return self.worksheets.get(sheet_id)
    
    def get_all_worksheets(self) -> Dict[int, Dict[str, Any]]:
        """
        Get all worksheets.
        
        Returns:
            dict: All worksheet data
        """
        return self.worksheets.copy()
    
    def get_worksheet_names(self) -> Dict[int, str]:
        """
        Get worksheet names mapped by ID.
        
        Returns:
            dict: Mapping of sheet_id -> name
        """
        return {sheet_id: data["name"] for sheet_id, data in self.worksheets.items()}
    
    def find_worksheet_by_name(self, name: str) -> Optional[int]:
        """
        Find worksheet ID by name (case-insensitive).
        
        Args:
            name (str): Name to search for
            
        Returns:
            int: Sheet ID or None if not found
        """
        name_lower = name.lower()
        for sheet_id, data in self.worksheets.items():
            if data["name"].lower() == name_lower:
                return sheet_id
        return None
    
    def save_to_file(self, file_path: str) -> bool:
        """
        Save all worksheets to a file.
        
        Args:
            file_path (str): Path to save the file
            
        Returns:
            bool: True if saved successfully
        """
        try:
            # Convert to the format expected by the original CalcForge
            data = {}
            for sheet_id, worksheet in self.worksheets.items():
                data[worksheet["name"]] = worksheet["content"]
            
            with open(file_path, 'w', encoding='utf-8') as f:
                json.dump(data, f, indent=2)
            
            return True
        except Exception as e:
            print(f"Error saving worksheets: {e}")
            return False
    
    def load_from_file(self, file_path: str) -> bool:
        """
        Load worksheets from a file.
        
        Args:
            file_path (str): Path to the file to load
            
        Returns:
            bool: True if loaded successfully
        """
        try:
            if not os.path.exists(file_path):
                return False
            
            with open(file_path, 'r', encoding='utf-8') as f:
                data = json.load(f)
            
            # Clear existing worksheets
            self.worksheets.clear()
            self.next_sheet_id = 1
            
            # Load worksheets from file
            for name, content in data.items():
                self.create_worksheet(name, content)
            
            return True
        except Exception as e:
            print(f"Error loading worksheets: {e}")
            return False
    
    def export_worksheet(self, sheet_id: int, file_path: str) -> bool:
        """
        Export a single worksheet to a file.
        
        Args:
            sheet_id (int): ID of the worksheet to export
            file_path (str): Path to save the file
            
        Returns:
            bool: True if exported successfully
        """
        worksheet = self.get_worksheet(sheet_id)
        if not worksheet:
            return False
        
        try:
            # Save as plain text or JSON based on file extension
            if file_path.endswith('.json'):
                with open(file_path, 'w', encoding='utf-8') as f:
                    json.dump({
                        "name": worksheet["name"],
                        "content": worksheet["content"]
                    }, f, indent=2)
            else:
                # Save as plain text
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(worksheet["content"])
            
            return True
        except Exception as e:
            print(f"Error exporting worksheet: {e}")
            return False
    
    def import_worksheet(self, file_path: str, name: str = None) -> Optional[int]:
        """
        Import a worksheet from a file.
        
        Args:
            file_path (str): Path to the file to import
            name (str): Name for the imported worksheet
            
        Returns:
            int: ID of the imported worksheet or None if failed
        """
        try:
            if not os.path.exists(file_path):
                return None
            
            if file_path.endswith('.json'):
                with open(file_path, 'r', encoding='utf-8') as f:
                    data = json.load(f)
                    content = data.get("content", "")
                    if name is None:
                        name = data.get("name", Path(file_path).stem)
            else:
                # Import as plain text
                with open(file_path, 'r', encoding='utf-8') as f:
                    content = f.read()
                if name is None:
                    name = Path(file_path).stem
            
            return self.create_worksheet(name, content)
            
        except Exception as e:
            print(f"Error importing worksheet: {e}")
            return None
    
    def get_cross_sheet_data(self) -> Dict[str, str]:
        """
        Get worksheet data formatted for cross-sheet references.
        
        Returns:
            dict: Mapping of sheet_name -> content
        """
        return {
            data["name"].lower(): data["content"] 
            for data in self.worksheets.values()
        }
