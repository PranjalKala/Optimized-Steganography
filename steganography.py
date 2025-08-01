import os
import subprocess
from PIL import Image
import numpy as np

class Steganography:
    def __init__(self):
        # Get the absolute path to code.exe
        current_dir = os.path.dirname(os.path.abspath(__file__))
        self.c_executable = os.path.join(current_dir, "code.exe")
        print(f"Using C executable at: {self.c_executable}")
        
    def encode(self, cover_image_path, secret_file_path, output_path):
        """
        Encode any file into the cover image
        """
        try:
            # Prepare input for the C program
            # The program expects:
            # 1. Choice (1 for encode)
            # 2. Input BMP path
            # 3. Secret file path
            # 4. Output path
            input_data = "1\n"  # Choose encode mode
            input_data += f"{cover_image_path}\n"  # Input BMP path
            input_data += f"{secret_file_path}\n"  # Secret file path
            input_data += f"{output_path}\n"  # Output path
            
            # Run the C program and capture output
            process = subprocess.Popen(
                [self.c_executable],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            
            # Send input and get output
            stdout, stderr = process.communicate(input=input_data)
            
            # Print the output for debugging
            print("Command output:", stdout)
            if stderr:
                print("Command errors:", stderr)
                
            if process.returncode == 0:
                return True
            else:
                print(f"Process returned with code: {process.returncode}")
                return False
                
        except Exception as e:
            print(f"Error during encoding: {e}")
            return False
            
    def decode(self, stego_image_path, output_path):
        """
        Decode the hidden file from the stego image
        """
        try:
            # Prepare input for the C program
            # The program expects:
            # 1. Choice (2 for decode)
            # 2. Stego image path
            # 3. Output path
            input_data = "2\n"  # Choose decode mode
            input_data += f"{stego_image_path}\n"  # Stego image path
            input_data += f"{output_path}\n"  # Output path
            
            # Run the C program and capture output
            process = subprocess.Popen(
                [self.c_executable],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            
            # Send input and get output
            stdout, stderr = process.communicate(input=input_data)
            
            # Print the output for debugging
            print("Command output:", stdout)
            if stderr:
                print("Command errors:", stderr)
                
            if process.returncode == 0:
                return True
            else:
                print(f"Process returned with code: {process.returncode}")
                return False
                
        except Exception as e:
            print(f"Error during decoding: {e}")
            return False
            
    def validate_cover_image(self, image_path):
        """
        Validate if the image is in BMP format
        """
        try:
            with Image.open(image_path) as img:
                if img.format != 'BMP':
                    return False, "Cover image must be in BMP format"
                return True, "Valid BMP image"
        except Exception as e:
            return False, str(e)
            
    def get_file_extension(self, filename):
        """
        Get the file extension from a filename
        """
        return os.path.splitext(filename)[1] 