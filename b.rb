#!/usr/bin/ruby
require 'bz2'
BZ2::Writer.open('b.rb.bz2') do |bz2|
   IO.foreach('b.rb') do |line|
      bz2.puts line
   end
end

BZ2::Reader.foreach("b.rb.bz2") do |line|
   puts line
end
