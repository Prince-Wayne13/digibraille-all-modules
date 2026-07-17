from pypdf import PdfReader, PdfWriter
import os

input_pdf = "chichewa dictionary.pdf"
reader = PdfReader(input_pdf)
total_pages = len(reader.pages)

# We want roughly 10 chunks to get ~16MB each
num_chunks = 10 
pages_per_chunk = total_pages // num_chunks

for i in range(num_chunks):
    writer = PdfWriter()
    start_page = i * pages_per_chunk
    # For the last chunk, grab all remaining pages
    end_page = total_pages if i == num_chunks - 1 else (i + 1) * pages_per_chunk 
    
    for page_num in range(start_page, end_page):
        writer.add_page(reader.pages[page_num])
    
    output_filename = f"dict_part_{i+1}.pdf"
    with open(output_filename, "wb") as out_file:
        writer.write(out_file)
    print(f"Created {output_filename} (Pages {start_page+1} to {end_page})")