# This file is mostly here for documentation purposes, do not require this

#
module Bzip2
  # A Bzip2::Writer represents a stream which compresses data written to it.
  # It can be constructed with another IO object (a File) which data can be
  # written to. Otherwise, data is all stored internally as a string and can
  # be retrieved via the Bzip2::Writer#flush method
  #
  # @see Bzip2::Writer#initialize The initialize method for examples
  class Writer

    alias :finish :flush

    # Append some data to this buffer, returning the buffer so this method can
    # be chained
    #
    #   writer = Bzip2::Writer.new
    #   writer << 'asdf' << 1 << obj << 'a'
    #   writer.flush
    #
    # @param [#to_s] data anything responding to #to_s
    # @see IO#<<
    def << data
    end

    # Adds a number of strings to this buffer. A newline is also inserted into
    # the buffer after each object
    # @see IO#puts
    def puts *objs
    end

    # Similar to Bzip2::Writer#puts except a newline is not appended after each
    # object appended to this buffer
    #
    # @see IO#print
    def print *objs
    end

    # Prints data to this buffer with the specified format.
    #
    # @see Kernel#sprintf
    def printf format, *ojbs
    end

  end
end
