# Concurrent-File-Operation-Extended-Reader-Writer-Problem-

Implemented multiple Readers-Writers problem using semaphores.

• Implemented the bounded waiting strategy to limit the starvation for the writer.

• The program should run until exit command is given as an input.

• A separate thread will be created for each read and write operation.

• The command to read will be of the format:

     read asd.txt
   
– On receiving this input, program will make a new thread for each reader to read the specified file. Also it will print a message containing the name of the file it read,
the size of the file, and how many readers and writers were present.

– The output format will be:

    read asd.txt of x bytes with n readers and m writers present
  
• The command to write can be of the following format:

     write 1 asd.txt efg.txt
     
     or
     
     write 2 asd.txt I am the text you need to add to asd.txt
   
• On receiving this input, program will make new a thread for each writer

  – If the command is write 1, then contents of the file efg.txt is written to file asd.txt 
  
  – If the command is write 2,  string present after the name of the text file is written to the file asd.txt
  
  – Also each writer print a message containing the name of the file it wrote to, the size of the write made, and how many readers and writers were present
  
  – The output format will be:
  
    writing to asd.txt added x bytes with n readers and m writers present

• An example input sequence can be:

    read asd.txt
    
    write 1 asd.txt sad.txt
    
    read asd.txt
    
    write 2 asd.txt 34324213
    
    read asd.txt
    
    read sad.txt
    
    read asd.txt
  
    write 1 asd.txt dfg.txt
    
    read asd.txt
    
    exit
  
