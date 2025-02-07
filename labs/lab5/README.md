

 **Three-Process Text Transformation Pipeline**

The purpose of this lab is to design and implement a three-stage text processing pipeline using processes and shared memory. You will develop three separate processes that communicate through shared memory segments to transform an input text file through a series of steps, demonstrating inter-process communication, synchronization, and shared memory management.

### **Task Description**

1. **Reader Process (Producer):**  
   - **Function:** Reads an input text file character by character.
   - **Transformation:**  
     - Every newline character (`\n`) encountered is replaced by the marker string `"<EOL>"` to preserve line breaks in a recognizable format.
   - **Shared Memory:**  
     - The processed characters are written into the first shared memory segment.
   - **Notes:**  
     - This process serves as the producer, ensuring that the entire input is available for further processing.

2. **Transformer Process (Squasher):**  
   - **Function:** Reads the contents of the first shared memory.
   - **Transformation:**  
     - Scans through the text and replaces every instance of adjacent asterisks (`**`) with a single hash character (`#`), thereby "squashing" these duplicate patterns.
   - **Shared Memory:**  
     - The resulting text is then written, character by character, into a second shared memory segment.
   - **Notes:**  
     - This stage cleans up the input data by consolidating repeated characters into a single, simplified symbol.

3. **Formatter Process (Printer):**  
   - **Function:** Reads the final transformed text from the second shared memory.
   - **Output Formatting:**  
     - The text is printed to the standard output in a neatly formatted manner, with each line consisting of exactly 20 characters.
   - **Notes:**  
     - This process is responsible for the final display, ensuring that the text output is consistent and easy to read.



### **Compilation and Execution Instructions**

- **Compilation:**  
  Compile the program using the standard GCC command. No special compile-time flags are necessary beyond those for threading if required (e.g., `-pthread`):
  ```bash
  gcc main.c -o main -pthread
  ```

- **Execution:**  
  Ensure that you have a proper text file available for processing (update the input file path in your source code if needed). Then run the executable:
  ```bash
  ./main
  ```

