import streamlit as st
import os
from steganography import Steganography
from PIL import Image
import tempfile
import time
import subprocess
import sys

st.set_page_config(
    page_title="File Steganography",
    page_icon="🔒",
    layout="wide"
)

st.title("File Steganography")
st.write("Hide any file within a BMP image using steganography")

# Initialize steganography object
stego = Steganography()

# Create tabs for different operations
tab1, tab2 = st.tabs(["Encode", "Decode"])

def run_command(command):
    try:
        # Create a status container
        status_container = st.empty()
        status_container.info("Processing...")
        
        # Create an expander for detailed information
        with st.expander("Processing Details", expanded=True):
            details_container = st.empty()
            details_container.info("Waiting for processing details...")
        
        # Determine if this is an encode or decode operation based on number of arguments
        is_encode = len(command) == 4  # encode has 4 args: exe, cover, secret, output
        
        # Prepare input for the C program
        input_data = "1\n" if is_encode else "2\n"  # 1 for encode, 2 for decode
        
        # Add the file paths
        if is_encode:
            input_data += f"{command[1]}\n"  # cover image
            input_data += f"{command[2]}\n"  # secret file
            input_data += f"{command[3]}\n"  # output
        else:
            input_data += f"{command[1]}\n"  # encoded image
            input_data += f"{command[2]}\n"  # output
        
        # Run the command with interactive input
        process = subprocess.Popen(
            [command[0]],  # Just the executable
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        # Send input and get output
        stdout, stderr = process.communicate(input=input_data)
        
        # Filter the output to only show relevant information
        if stdout:
            # Split the output into lines
            lines = stdout.split('\n')
            # Skip the first 7 lines (menu and prompts)
            filtered_output = '\n'.join(lines[7:])
            details_container.markdown("```\n" + filtered_output + "\n```")
        
        # Update status
        if process.returncode == 0:
            status_container.success("Process completed successfully!")
        else:
            status_container.error("Process failed!")
            if stderr:
                st.error(stderr.strip())
            
        return process.returncode == 0
        
    except Exception as e:
        status_container.error("Process failed!")
        st.error(f"Error: {str(e)}")
        return False

with tab1:
    st.header("Encode File")
    
    # File uploaders
    cover_image = st.file_uploader("Upload Cover Image (BMP)", type=['bmp'])
    secret_file = st.file_uploader("Upload File to Hide (Any Type)", type=None)
    
    if cover_image and secret_file:
        # Check file sizes
        cover_size = len(cover_image.getvalue())
        secret_size = len(secret_file.getvalue())
        
        if cover_size > 20*1024*1024:  # 20MB limit
            st.error("Cover image size exceeds 20MB limit")
            st.stop()
        if secret_size > 20*1024*1024:  # 20MB limit
            st.error("Secret file size exceeds 20MB limit")
            st.stop()
            
        # Create temporary files
        with tempfile.NamedTemporaryFile(delete=False, suffix='.bmp') as cover_temp:
            cover_temp.write(cover_image.getvalue())
            cover_path = cover_temp.name
            st.write(f"Cover image saved to: {cover_path}")
            
        with tempfile.NamedTemporaryFile(delete=False, suffix=stego.get_file_extension(secret_file.name)) as secret_temp:
            secret_temp.write(secret_file.getvalue())
            secret_path = secret_temp.name
            st.write(f"Secret file saved to: {secret_path}")
            
        # Validate cover image
        cover_valid, cover_msg = stego.validate_cover_image(cover_path)
        
        if not cover_valid:
            st.error(f"Cover image error: {cover_msg}")
        else:
            # Display cover image preview
            st.image(cover_image, caption="Cover Image")
            
            # Show file info
            file_size = len(secret_file.getvalue())
            st.info(f"File to hide: {secret_file.name} ({file_size} bytes)")
            
            if st.button("Encode File"):
                st.write("Starting encoding process...")
                
                with tempfile.NamedTemporaryFile(delete=False, suffix='.bmp') as output_temp:
                    output_path = output_temp.name
                    st.write(f"Output will be saved to: {output_path}")
                
                start_time = time.time()
                
                # Run the encoding process
                success = run_command([stego.c_executable, cover_path, secret_path, output_path])
                
                end_time = time.time()
                
                if success:
                    st.success(f"Encoding successful! (Time taken: {end_time - start_time:.2f} seconds)")
                    
                    # Provide download button for the encoded image
                    with open(output_path, 'rb') as f:
                        st.download_button(
                            label="Download Encoded Image",
                            data=f,
                            file_name="encoded_image.bmp",
                            mime="image/bmp"
                        )
                else:
                    st.error("Encoding failed!")
                
                # Cleanup temporary files
                try:
                    os.unlink(cover_path)
                    os.unlink(secret_path)
                    os.unlink(output_path)
                except Exception as e:
                    st.warning(f"Some temporary files could not be cleaned up: {str(e)}")

with tab2:
    st.header("Decode Hidden File")
    
    # File uploader for encoded image
    encoded_image = st.file_uploader("Upload Encoded Image (BMP)", type=['bmp'])
    
    if encoded_image:
        # Check file size
        encoded_size = len(encoded_image.getvalue())
        if encoded_size > 20*1024*1024:  # 20MB limit
            st.error("Encoded image size exceeds 20MB limit")
            st.stop()
            
        # Create temporary file
        with tempfile.NamedTemporaryFile(delete=False, suffix='.bmp') as encoded_temp:
            encoded_temp.write(encoded_image.getvalue())
            encoded_path = encoded_temp.name
            st.write(f"Encoded image saved to: {encoded_path}")
            
        # Validate image
        encoded_valid, encoded_msg = stego.validate_cover_image(encoded_path)
        
        if not encoded_valid:
            st.error(f"Encoded image error: {encoded_msg}")
        else:
            # Display preview
            st.image(encoded_image, caption="Encoded Image")
            
            if st.button("Decode File"):
                st.write("Starting decoding process...")
                
                with tempfile.NamedTemporaryFile(delete=False) as output_temp:
                    output_path = output_temp.name
                    st.write(f"Output will be saved to: {output_path}")
                
                start_time = time.time()
                
                # Run the decoding process
                success = run_command([stego.c_executable, encoded_path, output_path])
                
                end_time = time.time()
                
                if success:
                    st.success(f"Decoding successful! (Time taken: {end_time - start_time:.2f} seconds)")
                    
                    # Read the decoded file and provide download
                    with open(output_path, 'rb') as f:
                        file_data = f.read()
                        file_size = len(file_data)
                        
                        # Check if the file is empty or contains no valid data
                        if file_size == 0:
                            st.error("No hidden data found in the image. The image might not contain any encoded information.")
                        else:
                            # Try to determine file type from content
                            mime_type = 'application/octet-stream'
                            file_ext = '.bin'
                            
                            # Check for common file signatures
                            if file_data.startswith(b'\x89PNG'):
                                mime_type = 'image/png'
                                file_ext = '.png'
                            elif file_data.startswith(b'\xFF\xD8\xFF'):
                                mime_type = 'image/jpeg'
                                file_ext = '.jpg'
                            elif file_data.startswith(b'%PDF'):
                                mime_type = 'application/pdf'
                                file_ext = '.pdf'
                            elif file_data.startswith(b'PK\x03\x04'):
                                mime_type = 'application/zip'
                                file_ext = '.zip'
                            elif file_data.startswith(b'<!DOCTYPE html') or file_data.startswith(b'<html'):
                                mime_type = 'text/html'
                                file_ext = '.html'
                            elif file_data.startswith(b'<?xml'):
                                mime_type = 'application/xml'
                                file_ext = '.xml'
                            elif file_data.startswith(b'{\n') or file_data.startswith(b'{"'):
                                mime_type = 'application/json'
                                file_ext = '.json'
                            elif file_data.startswith(b'#') or file_data.startswith(b'//'):
                                mime_type = 'text/plain'
                                file_ext = '.txt'
                            
                            # If it's a text file (contains only printable ASCII characters)
                            if all(32 <= byte <= 126 or byte in (9, 10, 13) for byte in file_data):
                                mime_type = 'text/plain'
                                file_ext = '.txt'
                            
                            st.download_button(
                                label=f"Download Decoded File ({file_size} bytes)",
                                data=file_data,
                                file_name=f"decoded_file{file_ext}",
                                mime=mime_type
                            )
                else:
                    st.error("Decoding failed!")
                
                # Cleanup temporary files
                try:
                    os.unlink(encoded_path)
                    os.unlink(output_path)
                except Exception as e:
                    st.warning(f"Some temporary files could not be cleaned up: {str(e)}")

# Add footer
st.markdown("---")
st.markdown("### About")
st.markdown("""
This application uses steganography to hide any type of file within a BMP image.
- The cover image must be in BMP format
- You can hide any type of file (text, images, documents, etc.)
- The encoded image will be in BMP format
- The decoded file will maintain its original format
""") 