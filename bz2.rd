=begin
#^
bz2 is an extension to use bunzip2 from ruby
#^

#
# module BZ2
#
=== Module function

--- compress(str, blocks = 9, work = 0)
    Compress the String ((|str|))

    ((|blocks|)) specifies the block size to be used for compression.
    It should be a value between 1 and 9 inclusive, and the actual block
    size used is 100000 x this value

    ((|work|)) controls how the compression phase behaves when presented
    with worst case, highly repetitive, input data.Allowable values range 
    from 0 to 250 inclusive

--- uncompress(str, small = Qfalse)
    Uncompress the String ((|str|))

    If ((|small|)) is ((|true|)), the library will use an alternative 
    decompression algorithm which uses less memory but at the cost of
    decompressing more slowly

#
# # The class for compressing data
#class Writer
#

== BZ2::Writer
# class << self

=== Class methods

--- allocate
    allocate a new ((|BZ2::Writer|))

--- open(file, mode = "w", blocks = 9, work = 0) {|bz2| ... }
    Create a new ((|BZ2::Writer|)) and call the associated block.
    The object is closed at the end of the block

# end
=== Methods

--- close
    Terminate the compression.

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
    with worst case, highly repetitive, input data.Allowable values range 
    from 0 to 250 inclusive

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

--- write(str)
    Write the string ((|str|))

# end
# # The class for decompressing data. Data can be read directly from
# # a String, or from an object which must respond to read
#class Reader
# include Enumerable
#class << self

== BZ2::Reader

=== Class methods

--- allocate
    allocate a new ((|BZ2::Reader|))

--- foreach(filename, separator = $/) {|line| ... }
    Uncompress the file and call the block for each line, where
    lines are separated by ((|separator|))

--- open(filename)
    With no associated block, open is a synonym for BZ2::Reader::new. If the
    optional code block is given, it will be passed file as an
    argument, and the file will automatically be closed when the block
    terminates. In this instance, BZ2::Reader::open returns nil.

--- readlines(filename, separator = $/)
    Uncompress the file and reads the entire file as individual lines,
    and returns those lines in an array. Lines are separated by 
    ((|separator|))

#end
=== Methods

--- initialize(object, small = false)
    object must be a String which contains compressed data, or an
    object which respond to ((|read(size)|))

    If ((|small|)) is ((|true|)), the library will use an alternative 
    decompression algorithm which uses less memory but at the cost of
    decompressing more slowly

--- close(end = true)
    Terminate the uncompression.

    If ((|end|)) has a ((|true|)) value, the file is closed. Otherwise
    the file is put at the beginning of the next bzip component.

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

--- getc
    Get the next 8-bit byte (0..255). Returns nil if called
    at end of file.

--- gets(separator = $/)
    Reads the next line; lines are separated by ((|separator|)).
    Returns nil if called at end of file.

--- read(number)
    Read at most ((|number|)) characters
    Returns nil if called at end of file

--- readline(separator = $/)
    Reads the next line; lines are separated by ((|separator|)).
    Raise an error at end of file
     
--- readlines(separator = $/)
    Reads all of the lines, and returns them in anArray. Lines
    are separated by the optional ((|separator|))

--- ungetc(char)
    Push back one character

--- unused
    Return the String read by ((|BZ2::Reader|)) but not used in the 
    uncompression

--- unused=(str)
    Initialize the uncompression with the String ((|str|))

#end

== Exceptions

#
# #  Indicates that the library has been improperly compiled on your platform
# class ConfigError < ::Fatal
=== BZ2::ConfigError < Fatal
  Indicates that the library has been improperly compiled on your platform
# end

# # the superclass for all exceptions (except BZ2::ConfigError) raised by BZ2
# class Error < ::IOError
=== BZ2::Error < ::IOError
 the superclass for all exceptions (except BZ2::ConfigError) raised by BZ2
# end

# # "End of Zip" exception : compressed file finishes before the logical 
# # end of stream is detected
# class EOZError < Error
=== BZ2::EOZError < BZ2::Error
 "End of Zip" exception : compressed file finishes before the logical 
  end of stream is detected
# end

# # exception handled when an uncorrect sequence is detected (internal error)
# class SequenceError < Error
=== BZ2::SequenceError < BZ2::Error
 exception handled when an uncorrect sequence is detected (internal error)
# end

# # out of range for a parameter
# class ParamError < Error
=== BZ2::ParamError < BZ2::Error
 out of range for a parameter
# end

# # not enough memory is available
# class MemError < Error
=== BZ2::MemError < BZ2::Error
 not enough memory is available
# end

# # data integrity error is detected
# class DataError < Error
=== BZ2::DataError < BZ2::Error
 data integrity error is detected
# end

# # compressed stream does not start with the correct magic bytes
# class DataMagicError < Error
=== BZ2::DataMagicError < BZ2::Error
 compressed stream does not start with the correct magic bytes
# end

# # error reading or writing
# class IOError < Error
=== BZ2::IOError < BZ2::Error
 error reading or writing
# end

# # output buffer full
# class OutBuffFullError < Error
=== BZ2::OutBuffFullError < BZ2::Error
 output buffer full
# end
# end

=end
