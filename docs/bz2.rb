# bz2 is an extension to use libbzip2 from ruby
#
# classes implemented
#
# BZ2::Reader:: the class for decompressing data
# BZ2::Writer:: the class for compressing data
#
# BZ2::Error:: exception raised by BZ2
# BZ2::EOZError:: "End of Zip" exception
#
module BZ2

   class << self
      #Compress the String <em>str</em>
      #
      #<em>blocks</em> specifies the block size to be used for compression.
      #It should be a value between 1 and 9 inclusive, and the actual block
      #size used is 100000 x this value
      #
      #<em>work</em> controls how the compression phase behaves when presented
      #with worst case, highly repetitive, input data. 
      #
      #You should set this parameter carefully; too low, and many inputs
      #will be handled by the fallback algorithm and so compress rather
      #slowly, too high, and your average-to-worst case compression times
      #can become very large.
      #
      #Allowable values range from 0 to 250 inclusive. 0 is a special case,
      #equivalent to using the default value of 30.
      #
      #The default value of 30 gives reasonable behaviour over a wide
      #range of circumstances.
      #
      def  bzip2(str, blocks = 9, work = 0)
      end

      #same than <em> bzip2</em>
      def  compress(str, blocks = 9, work = 0)
      end

      #Uncompress the String <em>str</em>
      #
      #If <em>small</em> is <em>true</em>, the library will use an alternative 
      #decompression algorithm which uses less memory but at the cost of
      #decompressing more slowly
      #
      def  bunzip2(str, small = Qfalse)
      end

      #same than <em> bunzip2</em>
      def  uncompress(str, small = Qfalse)
      end
   end

   # The class for compressing data
   class Writer
      class << self

	 #Call Kernel#open(filename), associate it a new <em>BZ2::Writer</em>
	 #and call the associated block if given.
	 #
	 #The bz2 object is closed at the end of the block
	 #
	 #See <em>new</em> for <em>blocks</em> and <em>work</em>
	 #
	 def  open(filename, mode = "w", blocks = 9, work = 0) 
	    yield bz2
	 end
      end

      #Terminate the compression.
      #
      def  close
      end

      #Terminate the compression and close the associated object
      #
      def  close!
      end

      #Flush the data and terminate the compression, the object can be re-used
      #to store another compressed string
      #
      def  finish
      end

      #same than <em> finish</em>
      def  flush
      end

      #Create a new <em>BZ2::Writer</em> associated with <em>object</em>.
      #
      #<em>object</em> must respond to <em>write</em>, or must be <em>nil</em>
      #in this case the compressed string is returned when <em>flush</em>
      #is called
      #
      #If <em>object</em> is nil then the compression will be made in
      #a String which is returned when <em>close</em> or <em>flush</em> is called
      #
      #Otherwise <em>object</em> must respond to <em>write(str)</em>
      #
      #<em>blocks</em> specifies the block size to be used for compression.
      #It should be a value between 1 and 9 inclusive, and the actual block
      #size used is 100000 x this value
      #
      #<em>work</em> controls how the compression phase behaves when presented
      #with worst case, highly repetitive, input data. 
      #
      #You should set this parameter carefully; too low, and many inputs
      #will be handled by the fallback algorithm and so compress rather
      #slowly, too high, and your average-to-worst case compression times
      #can become very large.
      #
      #Allowable values range from 0 to 250 inclusive. 0 is a special case,
      #equivalent to using the default value of 30.
      #
      #The default value of 30 gives reasonable behaviour over a wide
      #range of circumstances.
      #
      def  initialize(object = nil, blocks = 9, work = 0)
      end
      
      #Writes <em>object</em>. <em>object</em> will be converted to a string using
      #to_s
      #
      def  <<(object)
      end
      
      #Writes the given object(s)
      #
      def  print(object = $_, ...)
      end
      
      #Formats and writes the given object(s)
      #
      def  printf(format, object = $_, ...)
      end
      
      #Writes the given character
      #
      def  putc(char)
      end
      
      #Writes the given objects 
      #
      def  puts(object, ...)
      end
      
      #return the associated object
      #
      def  to_io
      end
      
      #Write the string <em>str</em>
      #
      def  write(str)
      end
   end
   # The class for decompressing data. Data can be read directly from
   # a String, or from an object which must respond to read
   class Reader
      include Enumerable
      class << self
	 
	 #Uncompress the file and call the block for each line, where
	 #lines are separated by <em>separator</em>
	 #
	 def  foreach(filename, separator = $/) 
	    yield line
	 end
	 
	 #Call Kernel#open(filename), and associate it a new <em>BZ2::Reader</em>.
	 #The bz2 object is passed as an argument to the block.
	 #
	 #The object is closed at the end of the block
	 #
	 #See <em>new</em> for <em>small</em>
	 #
	 def  open(filename, small = false) 
	    yield bz2
	 end
	 
	 #Uncompress the file and reads the entire file as individual lines,
	 #and returns those lines in an array. Lines are separated by 
	 #<em>separator</em>
	 #
	 def  readlines(filename, separator = $/)
	 end
      end
      
      #Associate a new bz2 reader with <em>object</em>.
      #
      #object must be a String which contains compressed data, or an
      #object which respond to <em>read(size)</em>
      #
      #If <em>small</em> is <em>true</em>, the library will use an alternative 
      #decompression algorithm which uses less memory but at the cost of
      #decompressing more slowly
      #
      def  initialize(object, small = false)
      end
      
      #Terminate the uncompression and close the bz2
      #
      def  close
      end
      
      #Terminate the uncompression, close the bz2 and the associated object
      #
      def  close!
      end
      
      #Return true if the file is closed
      #
      def  closed?
      end
      
      #Execute the block for each line, where lines
      #are separated by the optional <em>separator</em>
      #
      def  each(separator = $/) 
	 yield line
      end
      #same than <em> each</em>
      def  each_line(separator = $/) 
	 yield line
      end
      
      #Return true at end of file
      #
      def  eof
      end
      
      #"End Of Zip". Return true at the end of the zip component
      #
      def  eoz
      end
      
      #Terminate the uncompression of the current zip component, and keep the
      #bz2 active (to read another zip component)
      #
      def  finish
      end
      
      #Get the next 8-bit byte (0..255). Returns nil if called
      #at end of file.
      #
      def  getc
      end
      
      #Reads the next line; lines are separated by <em>separator</em>.
      #Returns nil if called at end of file.
      #
      def  gets(separator = $/)
      end
      
      #Return the current line number
      #
      def  lineno
      end
      
      #Manually sets the current line number to the given value
      #
      def  lineno=(num)
      end
      
      #Read at most <em>number</em> characters
      #Returns nil if called at end of file
      #
      def  read(number)
      end
      
      #Reads the next line; lines are separated by <em>separator</em>.
      #Raise an error at end of file
      #     
      def  readline(separator = $/)
      end
      
      #Reads all of the lines, and returns them in anArray. Lines
      #are separated by the optional <em>separator</em>
      #
      def  readlines(separator = $/)
      end
      
      #return the associated object
      #
      def  to_io
      end
      
      #Push back one character
      #
      def  ungetc(char)
      end
      
      #Push back the string
      #
      def  ungets(str)
      end
      
      #Return the String read by <em>BZ2::Reader</em> but not used in the 
      #uncompression
      #
      def  unused
      end

      #Initialize the uncompression with the String <em>str</em>
      #
      def  unused=(str)
      end
   end

   #  Indicates that the library has been improperly compiled on your platform
   class ConfigError < ::Fatal
   end

   # Exception raised by BZ2
   class Error < ::IOError
   end

   # "End of Zip" exception : compressed file finishes before the logical
   # end of stream is detected
   class EOZError < Error
   end
end

