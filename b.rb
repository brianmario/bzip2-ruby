#!/usr/bin/ruby
require 'bz2'

File.open('b.rb.bz2', 'w') do |io|
   bz2 = BZ2::Writer.new(io)
   IO.foreach('b.rb') do |line|
      bz2.puts line
   end
   bz2.flush
   IO.foreach('b.rb') do |line|
      bz2.puts line
   end
   bz2.flush
   io.puts "abcdefghijklm"
end

BZ2::Reader.open("b.rb.bz2") do |bz2|
   while line = bz2.gets
      puts line
   end
   bz2.finish
   while line = bz2.gets
      puts line
   end
   p "eoz? = #{bz2.eoz?} -- eof? = #{bz2.eof?}"
   p bz2.unused
   p "eoz? = #{bz2.eoz?} -- eof? = #{bz2.eof?}"
end
