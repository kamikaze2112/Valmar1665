#!/usr/bin/env python3
import os
import re
from datetime import datetime

def update_app_version():
    # Get current date and time in the specified format
    now = datetime.now()
    version_string = now.strftime("%Y%m%d%H%M%S")
    
    # Path to globals.cpp (assuming it's in the src directory)
    globals_file = os.path.join("src", "globals.cpp")
    
    if not os.path.exists(globals_file):
        print(f"Error: {globals_file} not found!")
        return False
    
    try:
        # Read the file
        with open(globals_file, 'r', encoding='utf-8') as file:
            lines = file.readlines()
        
        # Find the //APP VERSION line and update the next APP_VERSION line
        found_comment = False
        updated = False
        
        for i, line in enumerate(lines):
            # Look for the //APP VERSION comment
            if line.strip() == "//APP VERSION":
                found_comment = True
                continue
            
            # If we found the comment, look for the next APP_VERSION line
            if found_comment and line.strip().startswith("const char* APP_VERSION"):
                # Replace this line with the new version
                lines[i] = f'const char* APP_VERSION = "{version_string}";\n'
                updated = True
                break
        
        if not found_comment:
            print("Error: //APP VERSION comment not found in globals.cpp")
            return False
        
        if not updated:
            print("Error: APP_VERSION line not found after //APP VERSION comment")
            return False
        
        # Write the file back
        with open(globals_file, 'w', encoding='utf-8') as file:
            file.writelines(lines)
        
        print(f"Successfully updated APP_VERSION to: {version_string}")
        return True
        
    except Exception as e:
        print(f"Error updating globals.cpp: {e}")
        return False

if __name__ == "__main__":
    success = update_app_version()
    exit(0 if success else 1)