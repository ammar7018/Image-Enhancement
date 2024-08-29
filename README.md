# Image-Enhancement
This application applies histogram equalization to an image, whose path (usually in the Input folder) is provided in the main function, and saves the processed image to the Output folder. It uses the MPI library to perform histogram equalization in parallel across multiple device cores, leveraging parallel programming techniques to reduce runtime.

## Purpose
This project is about using parallel processing to speed up histogram equalization for grayscale images. By using MPI (Message Passing Interface) with C++, we can split the work across several processing units, like CPU cores, to make the process faster.

## Tools Used
**`C++`**: The project uses C++ for its speed and flexibility in handling complex tasks.

**`MPI`**: MPI enables different computers or processes to work together simultaneously, making the project faster through parallel computing.

## Conclusion
By combining C++ with MPI, this project effectively accelerates the process of histogram equalization, making it more efficient and suitable for handling large-scale image processing tasks.

## Credits

[Ahmed Aboelassad](https://github.com/Ahmed-Aboalasaad)
[Ahmed Khaled]()
