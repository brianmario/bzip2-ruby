#!/usr/bin/ruby

# This is the test from rubicon

$LOAD_PATH.unshift(*%w{.. tests})
require 'bz2'
require 'runit_'

Inh = defined?(RUNIT) ? RUNIT : Test::Unit

$file = "_10lines_"
$data = [
   "00: This is a line\n",
   "01: This is a line\n",
   "02: This is a line\n",
   "03: This is a line\n",
   "04: This is a line\n",
   "05: This is a line\n",
   "06: This is a line\n",
   "07: This is a line\n",
   "08: This is a line\n",
   "09: This is a line\n" 
]

open("|bzip2 > #$file", "w") do |f|
   $data.each { |l| f.puts l }
end

class TestReader < Inh::TestCase

   SAMPLE = "08: This is a line\n"

   def test_f_s_foreach
      count = 0
      BZ2::Reader.foreach($file) do |line|
	 num = line[0..1].to_i
	 assert_equal(count, num)
	 count += 1
      end
      assert_equal($data.size, count)
      
      count = 0
      BZ2::Reader.foreach($file, nil) do |file|
	 file.split(/\n/).each do |line|
	    num = line[0..1].to_i
	    assert_equal(count, num)
	    count += 1
	 end
      end
      assert_equal($data.size, count)

      count = 0
      BZ2::Reader.foreach($file, ' ') do |thing|
	 count += 1
      end
      assert_equal(41, count)
   end

   def test_f_s_readlines
      lines = BZ2::Reader.readlines($file)
      assert_equal($data.size, lines.size)

      lines = BZ2::Reader.readlines($file, nil)
      assert_equal(1, lines.size)
      assert_equal(SAMPLE.length * $data.size, lines[0].size)
   end

   def test_f_closed?
      f = BZ2::Reader.open($file)
      assert(!f.closed?)
      f.close
      assert(f.closed?)
   end

   def test_f_each
      count = 0
      BZ2::Reader.open($file) do |file|
	 file.each do |line|
	    num = line[0..1].to_i
	    assert_equal(count, num)
	    count += 1
	 end
	 assert_equal($data.size, count)
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
      assert_equal($data.size, count)

      count = 0
      BZ2::Reader.open($file) do |file|
	 file.each(' ') do |thing|
	    count += 1
	 end
      end
      assert_equal(41, count)
   end

   def test_f_each_byte
      count = 0
      data = $data.join

      BZ2::Reader.open($file) do |file|
	 file.each_byte do |b|
	    assert_equal(data[count], b)
	    count += 1
	 end
      end
      assert_equal(SAMPLE.length * $data.size, count)
   end

   def test_f_each_line
      count = 0
      BZ2::Reader.open($file) do |file|
	 file.each_line do |line|
	    num = line[0..1].to_i
	    assert_equal(count, num)
	    count += 1
	 end
	 assert_equal($data.size, count)
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
      assert_equal($data.size, count)

      count = 0
      BZ2::Reader.open($file) do |file|
	 file.each_line(' ') do |thing|
	    count += 1
	 end
      end
      assert_equal(41, count)
   end

   def test_f_eof
      BZ2::Reader.open($file) do |file|
	 $data.size.times do
	    assert(!file.eof)
	    assert(!file.eof?)
	    file.gets
	 end
	 assert(file.eof)
	 assert(file.eof?)
      end
   end

   def test_f_getc
      count = 0
      data = $data.join
      
      BZ2::Reader.open($file) do |file|
	 while (ch = file.getc)
	    assert_equal(data[count], ch)
	    count += 1
	 end
	 assert_equal(nil, file.getc)
      end
      assert_equal(SAMPLE.length * $data.size, count)
   end

   def test_f_gets
      count = 0
      BZ2::Reader.open($file) do |file|
	 while (line = file.gets)
	    num = line[0..1].to_i
	    assert_equal(count, num)
	    count += 1
	 end
	 assert_equal(nil, file.gets)
	 assert_equal($data.size, count)
      end

      count = 0
      BZ2::Reader.open($file) do |file|
	 while (line = file.gets("line\n"))
	    assert_equal($data[count], line)
	    num = line[0..1].to_i
	    assert_equal(count, num)
	    count += 1
	 end
	 assert_equal(nil, file.gets)
	 assert_equal($data.size, count)
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
      assert_equal($data.size, count)

      count = 0
      BZ2::Reader.open($file) do |file|
	 while (thing = file.gets(' '))
	    count += 1
	 end
      end
      assert_equal(41, count)
   end

   def test_f_read
      BZ2::Reader.open($file) do |file|
	 content = file.read
	 assert_equal(SAMPLE.length * $data.size, content.length)
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

   def test_f_readchar
      count = 0
      data = $data.join
      BZ2::Reader.open($file) do |file|
	 190.times do |count|
	    ch = file.readchar
	    assert_equal(data[count], ch)
	    count += 1
	 end
	 assert_raises(BZ2::EOZError) { file.readchar }
      end
   end

   def test_f_readline
      count = 0
      BZ2::Reader.open($file) do |file|
	 $data.size.times do |count|
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
      assert_equal($data.size, count)

      count = 0
      BZ2::Reader.open($file) do |file|
	 41.times do |count|
	    thing = file.readline(' ')
	    count += 1
	 end
	 assert_raises(BZ2::EOZError) { file.readline }
      end
   end

   def test_f_readlines
      BZ2::Reader.open($file) do |file|
	 lines = file.readlines
	 assert_equal($data.size, lines.size)
      end
      
      BZ2::Reader.open($file) do |file|
	 lines = file.readlines(nil)
	 assert_equal(1, lines.size)
	 assert_equal(SAMPLE.length * $data.size, lines[0].size)
      end
   end

   def test_f_ungetc
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
   
   def test_f_ungets
      count = 0
      BZ2::Reader.open($file) do |file|
	 assert_equal($data[count], file.gets)
	 assert_equal(count + 1, file.lineno);
	 assert_nil(file.ungets($data[count]))
	 assert_equal($data[count], file.gets)
	 count += 1
      end
   end

   def test_s_readline
      count = 0
      string = IO.readlines($file, nil)[0]
      file = BZ2::Reader.new(string)
      $data.size.times do |count|
	 line = file.readline
	 num = line[0..1].to_i
	 assert_equal(count, num)
	 count += 1
      end
      assert_raises(BZ2::EOZError) { file.readline }
      file.close

      count = 0
      file = BZ2::Reader.new(string)
      contents = file.readline(nil)
      contents.split(/\n/).each do |line|
	 num = line[0..1].to_i
	 assert_equal(count, num)
	 count += 1
      end
      assert_raises(BZ2::EOZError) { file.readline }
      assert_equal($data.size, count)
      file.close

      count = 0
      file = BZ2::Reader.new(string)
      41.times do |count|
	 thing = file.readline(' ')
	 count += 1
      end
      assert_raises(BZ2::EOZError) { file.readline }
      file.close
   end

   def test_zzz
      File.unlink($file)
   end
end


if defined?(RUNIT)
   RUNIT::CUI::TestRunner.run(TestReader.suite)
end
