=begin

bz2 is an extension to use libbzip2 from ruby

=== Module function

--- bzip2(str, blocks = 9, work = 0)
--- compress(str, blocks = 9, work = 0)
    Compress the String ((|str|))

    ((|blocks|)) specifies the block size to be used for compression.
    It should be a value between 1 and 9 inclusive, and the actual block
    size used is 100000 x this value

    ((|work|)) controls how the compression phase behaves when presented
    with worst case, highly repetitive, input data. 

    You should set this parameter carefully; too low, and many inputs
    will be handled by the fallback algorithm and so compress rather
    slowly, too high, and your average-to-worst case compression times
    can become very large.

    Allowable values range from 0 to 250 inclusive. 0 is a special case,
    equivalent to using the default value of 30.

    The default value of 30 gives reasonable behaviour over a wide
    range of circumstances.

--- bunzip2(str, small = Qfalse)
--- uncompress(str, small = Qfalse)
    Uncompress the String ((|str|))

    If ((|small|)) is ((|true|)), the library will use an alternative 
    decompression algorithm which uses less memory but at the cost of
    decompressing more slowly

== BZ2::Writer

=== Class methods

--- allocate
    allocate a new ((|BZ2::Writer|))

--- new(object = nil, mode = "w", blocks = 9, work = 0)
    Create a new ((|BZ2::Writer|)) associated with ((|object|)).

    ((|object|)) must respond to ((|write|)), or must be ((|nil|))
    in this case the compressed string is returned when ((|flush|))
    is called

    See ((|initialize|)) for ((|blocks|)) and ((|work|))

--- open(filename, mode = "w", blocks = 9, work = 0) {|bz2| ... }
    Call Kernel#open(filename), associate it a new ((|BZ2::Writer|))
    and call the associated block if given.

    The bz2 object is closed at the end of the block

    See ((|initialize|)) for ((|blocks|)) and ((|work|))

=== Methods

--- close
    Terminate the compression.

--- close!
    Terminate the compression and close the associated object

--- finish
--- flush
    Flush the data and terminate the compression, the object can be re-used
    to store another compressed string

--- initialize(object = nil, blocks = 9, work = 0)
    If ((|object|)) is nil then the compression will be made in
    a String which is returned when ((|close|)) or ((|flush|)) is called

    Otherwise ((|object|)) must respond to ((|write(str)|))

    ((|blocks|)) specifies the block size to be used for compression.
    It should be a value between 1 and 9 inclusive, and the actual block
    size used is 100000 x this value

    ((|work|)) controls how the compression phase behaves when presented
    with worst case, highly repetitive, input data. 

    You should set this parameter carefully; too low, and many inputs
    will be handled by the fallback algorithm and so compress rather
    slowly, too high, and your average-to-worst case compression times
    can become very large.

    Allowable values range from 0 to 250 inclusive. 0 is a special case,
    equivalent to using the default value of 30.

    The default value of 30 gives reasonable behaviour over a wide
    range of circumstances.

--- <<(object)
    Writes ((|object|)). ((|object|)) will be converted to a string using
    to_s

--- print(object = $_, ...)
    Writes the given object(s)

--- printf(format, object = $_, ...)
    Formats and writes the given object(s)

--- putc(char)
    Writes the given character

--- puts(object, ...)
    Writes the given objects 

--- to_io
    return the associated object

--- write(str)
    Write the string ((|str|))

== BZ2::Reader

 Included modules : Enumerable

=== Class methods

--- allocate
    allocate a new ((|BZ2::Reader|))

--- foreach(filename, separator = $/) {|line| ... }
    Uncompress the file and call the block for each line, where
    lines are separated by ((|separator|))

--- new(object, small = false)
    Associate a new bz2 reader with ((|object|)). ((|object|)) must
    respond to ((|read|))

    See ((|initialize|)) for ((|small|))

--- open(filename, small = false) {|bz2| ... }
    Call Kernel#open(filename), and associate it a new ((|BZ2::Reader|)).
    The bz2 object is passed as an argument to the block.

    The object is closed at the end of the block

    See ((|initialize|)) for ((|small|))

--- readlines(filename, separator = $/)
    Uncompress the file and reads the entire file as individual lines,
    and returns those lines in an array. Lines are separated by 
    ((|separator|))

=== Methods

--- initialize(object, small = false)
    object must be a String which contains compressed data, or an
    object which respond to ((|read(size)|))

    If ((|small|)) is ((|true|)), the library will use an alternative 
    decompression algorithm which uses less memory but at the cost of
    decompressing more slowly

--- close
    Terminate the uncompression and close the bz2

--- close!
    Terminate the uncompression, close the bz2 and the associated object

--- closed?
    Return true if the file is closed

--- each(separator = $/) {|line| ... }
--- each_line(separator = $/) {|line| ... }
    Execute the block for each line, where lines
    are separated by the optional ((|separator|))

--- eof
    Return true at end of file

--- eoz
    "End Of Zip". Return true at the end of the zip component

--- finish
    Terminate the uncompression of the current zip component, and keep the
    bz2 active (to read another zip component)

--- getc
    Get the next 8-bit byte (0..255). Returns nil if called
    at end of file.

--- gets(separator = $/)
    Reads the next line; lines are separated by ((|separator|)).
    Returns nil if called at end of file.

--- lineno
    Return the current line number

--- lineno=(num)
    Manually sets the current line number to the given value

--- read(number)
    Read at most ((|number|)) characters
    Returns nil if called at end of file

--- readline(separator = $/)
    Reads the next line; lines are separated by ((|separator|)).
    Raise an error at end of file
     
--- readlines(separator = $/)
    Reads all of the lines, and returns them in anArray. Lines
    are separated by the optional ((|separator|))

--- to_io
    return the associated object

--- ungetc(char)
    Push back one character

--- ungets(str)
    Push back the string

--- unused
    Return the String read by ((|BZ2::Reader|)) but not used in the 
    uncompression

--- unused=(str)
    Initialize the uncompression with the String ((|str|))

== Exceptions

=== BZ2::ConfigError < Fatal
  Indicates that the library has been improperly compiled on your platform

=== BZ2::Error < ::IOError
 Exception raised by BZ2

=== BZ2::EOZError < BZ2::Error
 "End of Zip" exception : compressed file finishes before the logical 
  end of stream is detected

=end
