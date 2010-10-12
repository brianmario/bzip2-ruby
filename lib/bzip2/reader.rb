# This file is mostly here for documentation purposes, do not require this

#
module Bzip2
  # Bzip2::Reader is meant to read streams of bz2 compressed bytes. It behaves
  # like an IO object with many similar methods. It also includes the Enumerable
  # module and each element is a 'line' in the stream.
  #
  # It can both decompress files:
  #
  #     reader = Bzip2::Reader.open('file')
  #     puts reader.read
  #
  #     reader = Bzip2::Reader.new File.open('file')
  #     put reader.gets
  #
  # And it may just decompress raw strings
  #
  #     reader = Bzip2::Reader.new compressed_string
  #     reader = Bzip2::Reader.new Bzip2.compress('compress-me')
  class Reader
    alias :each_line :each
    alias :closed :closed?
    alias :eoz :eoz?
    alias :eof :eof?
  end
end
