import re

def process_word_list(input_file, output_file):
    with open(input_file, 'r', encoding='utf-8') as f:
        text = f.read()

    # 1. Remove all tags (Markdown headers, bold text, horizontal rules)
    text = re.sub(r'#+.*', '', text)
    text = re.sub(r'\*\*.*?\*\*', '', text)
    text = re.sub(r'---+', '', text)
    
    # 2. Remove commas (replace with newlines to separate words)
    text = text.replace(',', '\n')
    
    # Split into individual lines/words
    raw_words = text.split()
    
    cleaned_words = set()
    for word in raw_words:
        # Strip any remaining edge punctuation (keeps internal hyphens)
        clean_word = re.sub(r'^[^a-zA-Z0-9-]+|[^a-zA-Z0-9-]+$', '', word)
        if clean_word:
            # Convert to lowercase and add to set to automatically remove duplicates
            cleaned_words.add(clean_word.lower())
            
    # 3. Arrange alphabetically
    sorted_words = sorted(list(cleaned_words))
    
    # 4. Save in the exact format (one word per line)
    with open(output_file, 'w', encoding='utf-8') as f:
        for word in sorted_words:
            f.write(word + '\n')
            
    print(f"Success! {len(sorted_words)} unique words saved to '{output_file}'.")

# Run the function (make sure your file is in the same directory)
process_word_list('english_common_words_5000.txt', 'english_common_words_6000.txt')