#!/usr/bin/ruby

# This is the test from rubicon

$LOAD_PATH.unshift *%w{.. tests}
require 'bz2'
require 'runit_'

Inh = defined?(RUNIT) ? RUNIT : Test::Unit

$file = "_10lines_"

open("|bzip2 > #$file", "w") do |f|
   10.times { |i| f.printf "%02d: This is a line\n", i }
end

class TestReader < Inh::TestCase

   SAMPLE = "08: This is a line\n"

   def test_s_foreach
      count = 0
      BZ2::Reader.foreach($file) do |line|
	 num = line[0..1].to_i
	 assert_equal(count, num)
	 count += 1
      end
      assert_equal(10, count)
      
      count = 0
      BZ2::Reader.foreach($file, nil) do |file|
	 file.split(/\n/).each do |line|
	    num = line[0..1].to_i
	    assert_equal(count, num)
	    count += 1
	 end
      end
      assert_equal(10, count)

      count = 0
      BZ2::Reader.foreach($file, ' ') do |thing|
	 count += 1
      end
      assert_equal(41, count)
   end

   def test_s_readlines
      lines = BZ2::Reader.readlines($file)
      assert_equal(10, lines.size)

      lines = BZ2::Reader.readlines($file, nil)
      assert_equal(1, lines.size)
      assert_equal(SAMPLE.length*10, lines[0].size)
   end

   def test_closed?
      f = BZ2::Reader.open($file)
      assert(!f.closed?)
      f.close
      assert(f.closed?)
   end

   def test_each
      count = 0
      BZ2::Reader.open($file) do |file|
	 file.each do |line|
	    num = line[0..1].to_i
	    assert_equal(count, num)
	    count += 1
	 end
	 assert_equal(10, count)
      end

      count = 0
      BZ2::Reader.open($file) do |file|
	 file.each(nil) do |contents|
	    contents.split(/\n/).each do |line|
	       num = line[0..1].to_i
	       assert_equal(count, num)
	       count += 1
	    end
	 end
      end
      assert_equal(10, count)

      count = 0
      BZ2::Reader.open($file) do |file|
	 file.each(' ') do |thing|
	    count += 1
	 end
      end
      assert_equal(41, count)
   end

   def test_each_byte
      count = 0
      data = 
	 "00: This is a line\n" +
	 "01: This is a line\n" +
	 "02: This is a line\n" +
	 "03: This is a line\n" +
	 "04: This is a line\n" +
	 "05: This is a line\n" +
	 "06: This is a line\n" +
	 "07: This is a line\n" +
	 "08: This is a line\n" +
	 "09: This is a line\n" 

      BZ2::Reader.open($file) do |file|
	 file.each_byte do |b|
	    assert_equal(data[count], b)
	    count += 1
	 end
      end
      assert_equal(SAMPLE.length*10, count)
   end

   def test_each_line
      count = 0
      BZ2::Reader.open($file) do |file|
	 file.each_line do |line|
	    num = line[0..1].to_i
	    assert_equal(count, num)
	    count += 1
	 end
	 assert_equal(10, count)
      end

      count = 0
      BZ2::Reader.open($file) do |file|
	 file.each_line(nil) do |contents|
	    contents.split(/\n/).each do |line|
	       num = line[0..1].to_i
	       assert_equal(count, num)
	       count += 1
	    end
	 end
      end
      assert_equal(10, count)

      count = 0
      BZ2::Reader.open($file) do |file|
	 file.each_line(' ') do |thing|
	    count += 1
	 end
      end
      assert_equal(41, count)
   end

   def test_eof
      BZ2::Reader.open($file) do |file|
	 10.times do
	    assert(!file.eof)
	    assert(!file.eof?)
	    file.gets
	 end
	 assert(file.eof)
	 assert(file.eof?)
      end
   end

   def test_getc
      count = 0
      data = 
	 "00: This is a line\n" +
	 "01: This is a line\n" +
	 "02: This is a line\n" +
	 "03: This is a line\n" +
	 "04: This is a line\n" +
	 "05: This is a line\n" +
	 "06: This is a line\n" +
	 "07: This is a line\n" +
	 "08: This is a line\n" +
	 "09: This is a line\n" 
      
      BZ2::Reader.open($file) do |file|
	 while (ch = file.getc)
	    assert_equal(data[count], ch)
	    count += 1
	 end
	 assert_equal(nil, file.getc)
      end
      assert_equal(SAMPLE.length*10, count)
   end

   def test_gets
      count = 0
      BZ2::Reader.open($file) do |file|
	 while (line = file.gets)
	    num = line[0..1].to_i
	    assert_equal(count, num)
	    count += 1
	 end
	 assert_equal(nil, file.gets)
	 assert_equal(10, count)
      end

      count = 0
      BZ2::Reader.open($file) do |file|
	 while (contents = file.gets(nil))
	    contents.split(/\n/).each do |line|
	       num = line[0..1].to_i
	       assert_equal(count, num)
	       count += 1
	    end
	 end
      end
      assert_equal(10, count)

      count = 0
      BZ2::Reader.open($file) do |file|
	 while (thing = file.gets(' '))
	    count += 1
	 end
      end
      assert_equal(41, count)
   end

   def test_read
      BZ2::Reader.open($file) do |file|
	 content = file.read
	 assert_equal(SAMPLE.length*10, content.length)
	 count = 0
	 content.split(/\n/).each do |line|
	    num = line[0..1].to_i
	    assert_equal(count, num)
	    count += 1
	 end
      end

      BZ2::Reader.open($file) do |file|
	 assert_equal("00: This is ", file.read(12))
	 assert_equal("a line\n01: T", file.read(12))
      end
   end

   def test_readchar
      count = 0
      data = 
	 "00: This is a line\n" +
	 "01: This is a line\n" +
	 "02: This is a line\n" +
	 "03: This is a line\n" +
	 "04: This is a line\n" +
	 "05: This is a line\n" +
	 "06: This is a line\n" +
	 "07: This is a line\n" +
	 "08: This is a line\n" +
	 "09: This is a line\n" 
      
      BZ2::Reader.open($file) do |file|
	 190.times do |count|
	    ch = file.readchar
	    assert_equal(data[count], ch)
	    count += 1
	 end
	 assert_raises(BZ2::EOZError) { file.readchar }
      end
   end

   def test_readline
      count = 0
      BZ2::Reader.open($file) do |file|
	 10.times do |count|
	    line = file.readline
	    num = line[0..1].to_i
	    assert_equal(count, num)
	    count += 1
	 end
	 assert_raises(BZ2::EOZError) { file.readline }
      end

      count = 0
      BZ2::Reader.open($file) do |file|
	 contents = file.readline(nil)
	 contents.split(/\n/).each do |line|
	    num = line[0..1].to_i
	    assert_equal(count, num)
	    count += 1
	 end
	 assert_raises(BZ2::EOZError) { file.readline }
      end
      assert_equal(10, count)

      count = 0
      BZ2::Reader.open($file) do |file|
	 41.times do |count|
	    thing = file.readline(' ')
	    count += 1
	 end
	 assert_raises(BZ2::EOZError) { file.readline }
      end
   end

   def test_readlines
      BZ2::Reader.open($file) do |file|
	 lines = file.readlines
	 assert_equal(10, lines.size)
      end
      
      BZ2::Reader.open($file) do |file|
	 lines = file.readlines(nil)
	 assert_equal(1, lines.size)
	 assert_equal(SAMPLE.length*10, lines[0].size)
      end
   end

   def test_ungetc
      BZ2::Reader.open($file) do |file|
	 assert_equal(?0, file.getc)
	 assert_equal(?0, file.getc)
	 assert_equal(?:, file.getc)
	 assert_equal(?\s, file.getc)
	 assert_nil(file.ungetc(?:))
	 assert_equal(?:, file.getc)
	 1 while file.getc
	 assert_nil(file.ungetc(?A))
	 assert_equal(?A, file.getc)
      end
   end

   def test_zzz
      File.unlink($file)
   end
end


if defined?(RUNIT)
   RUNIT::CUI::TestRunner.run(TestIO.suite)
end
