#ifndef jt_FILE_STATISTICS_h
#define jt_FILE_STATISTICS_h

/** 
    statics (lines of code) from a file
*/
struct file_statistics
{
    file_statistics();

    int commented;
    int empty;
    int code;
    int total;
    
    // this contains the nubmer of chars, once each line is trimmed
    unsigned long non_space_chars;

    void operator+=(const file_statistics& to_add);
};

#endif

