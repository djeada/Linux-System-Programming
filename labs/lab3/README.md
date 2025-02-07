**Build Your Own Shell**

#### **Objective**

- **Learn Process Creation and Execution:**  
  Gain hands-on experience with process management in Linux by creating a program that can execute commands much like a typical Unix shell.
  
- **Practice String Parsing:**  
  Improve your skills in tokenizing input strings, which is essential for interpreting and executing user commands.
  
- **Understand `execvp`:**  
  Learn how to use the `execvp` function to replace the current process image with a new process image specified by the user’s command.

#### **Task Description**

Your shell program should perform the following steps:

1. **Prompting the User:**  
   - Continuously display a prompt (e.g., `myshell> `) asking the user to enter a command.
   - **Important Note:** When entering a command, make sure to include a space at the end. This is necessary to ensure that the tokenization works correctly.

2. **Reading the Input:**  
   - Read the full command line from the user input.
   - Handle input safely and ensure that you account for buffer overflow issues.

3. **Tokenizing the Command:**  
   - Break the input command into tokens (e.g., the command itself and its arguments).
   - You may use functions such as `strtok()` to separate the command string using whitespace as the delimiter.

4. **Executing the Command:**  
   - Use the `fork()` system call to create a child process.
   - In the child process, use `execvp` to execute the command with the provided arguments.
   - The parent process should wait for the child process to complete before prompting for the next command.

5. **Error Handling:**  
   - If the command cannot be executed (for example, if the command does not exist), print an appropriate error message.
   - Ensure that your shell does not crash on invalid input and continues running until the user decides to exit (e.g., by typing `exit `).

6. **Exiting the Shell:**  
   - Provide a way for the user to exit the shell gracefully (commonly by typing `exit `).

#### **Additional Features (Optional but Recommended)**

- **Built-in Commands:**  
  Consider implementing a few built-in commands like `cd` (change directory) and `help` that do not rely on `execvp`, to further enhance your shell.
  
- **History and Re-Execution:**  
  You might expand your shell to maintain a simple history of commands that can be re-executed using a special syntax (for example, `!!` to re-run the last command).

- **Signal Handling:**  
  Optionally, handle signals (like `Ctrl+C`) so that your shell does not exit unexpectedly on user interrupts.

#### **Compilation and Execution Instructions**

1. **Compilation:**
   - Use the GNU Compiler Collection (gcc) with the basic command.
   - Example compile command:
     ```bash
     gcc your_shell_program.c -o myshell
     ```
   - No additional libraries or compile-time flags are required.

2. **Execution:**
   - Run your shell by executing the produced binary.
     ```bash
     ./myshell
     ```
   - Once running, the shell will display a prompt (e.g., `myshell> `).  
   **Note:** Always remember to type a space at the end of your command when prompted. For example, to list directory contents, type:
     ```
     ls -l 
     ```
     The trailing space ensures that your command gets parsed correctly.

#### **Tips for Development**

- **Modularize Your Code:**  
  Break your program into functions, for example:
  - A function to display the prompt and read input.
  - A function to tokenize the input string.
  - A function to execute the tokenized command.
  
- **Testing:**  
  Test with various Linux commands (both built-in and external) to ensure your shell behaves as expected.
  
- **Debugging:**  
  Use print statements or a debugger (like `gdb`) to trace the flow of your program and catch any parsing or execution errors.

- **Comment and Document:**  
  Include comments throughout your code to explain your logic and ensure the code is readable for both yourself and others.
