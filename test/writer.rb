#!/usr/bin/ruby

# This is the test from rubicon

$LOAD_PATH.unshift(*%w{.. tests})
require 'bz2'
require 'runit_'

Inh = defined?(RUNIT) ? RUNIT : Test::Unit

$file = "_10lines_"

class Dummy
   def to_s
      "dummy"
   end
end

class TestWriter < Inh::TestCase

   def test_f_LSHIFT # '<<'
      BZ2::Writer.open($file, "w") do |file|
	 file << 1 << "\n" << Dummy.new << "\n" << "cat\n"
      end
      expected = [ "1\n", "dummy\n", "cat\n"]
      BZ2::Reader.foreach($file) do |line|
	 assert_equal(expected.shift, line)
      end
      assert_equal([], expected)
   end

   def test_f_error
      io = File.new($file, "w")
      bz2 = BZ2::Writer.new(io)
      bz2 << 1 << "\n" << Dummy.new << "\n" << "cat\n"
      bz = BZ2::Reader.new($file)
      assert_raises(BZ2::Error) { bz.gets }
      bz = BZ2::Reader.open($file)
      assert_raises(BZ2::EOZError) { bz.gets }
      io.close
      assert_raises(IOError) { BZ2::Reader.new(io) }
   end

   def test_f_gets_para
      BZ2::Writer.open($file) do |file|
	 file.print "foo\n"*4096, "\n"*4096, "bar"*4096, "\n"*4096, "zot\n"*1024
      end
      BZ2::Reader.open($file) do |file|
	 assert_equal("foo\n"*4096+"\n", file.gets(""))
	 assert_equal("bar"*4096+"\n\n", file.gets(""))
	 assert_equal("zot\n"*1024, file.gets(""))
      end
   end

   def test_f_print
      BZ2::Writer.open($file) do |file|
	 file.print "hello"
	 file.print 1,2
	 $_ = "wombat\n"
	 file.print
	 $\ = ":"
	 $, = ","
	 file.print 3, 4
	 file.print 5, 6
	 $\ = nil
	 file.print "\n"
	 $, = nil
      end

      BZ2::Reader.open($file) do |file|
	 content = file.gets(nil)
	 assert_equal("hello12wombat\n3,4:5,6:\n", content)
      end
   end

   def test_f_putc
      BZ2::Writer.open($file, "wb") do |file|
	 file.putc "A"
	 0.upto(255) { |ch| file.putc ch }
      end

      BZ2::Reader.open($file, "rb") do |file|
	 assert_equal(?A, file.getc)
	 0.upto(255) { |ch| assert_equal(ch, file.getc) }
      end
   end

   def test_f_puts
      BZ2::Writer.open($file, "w") do |file|
	 file.puts "line 1", "line 2"
	 file.puts [ Dummy.new, 4 ]
      end

      BZ2::Reader.open($file) do |file|
	 assert_equal("line 1\n",  file.gets)
	 assert_equal("line 2\n",  file.gets)
	 assert_equal("dummy\n",   file.gets)
	 assert_equal("4\n",       file.gets)
      end
   end

   def test_f_write
      BZ2::Writer.open($file, "w") do |file|
	 assert_equal(10, file.write('*' * 10))
	 assert_equal(5,  file.write('!' * 5))
	 assert_equal(0,  file.write(''))
	 assert_equal(1,  file.write(1))
	 assert_equal(3,  file.write(2.30000))
	 assert_equal(1,  file.write("\n"))
      end
    
      BZ2::Reader.open($file) do |file|
	 assert_equal("**********!!!!!12.3\n", file.gets)
      end
   end

   def test_s_string
      file = BZ2::Writer.new
      assert_equal(10, file.write('*' * 10))
      assert_equal(5,  file.write('!' * 5))
      assert_equal(0,  file.write(''))
      assert_equal(1,  file.write(1))
      assert_equal(3,  file.write(2.30000))
      assert_equal(1,  file.write("\n"))
      line = BZ2::bunzip2(file.flush)
      assert_equal("**********!!!!!12.3\n", line)

      line = BZ2::bunzip2(BZ2::bzip2("**********!!!!!12.3\n"))
      assert_equal("**********!!!!!12.3\n", line)

      test = "foo\n"*4096 + "\n"*4096 + "bar"*4096 + "\n"*4096 + "zot\n"*1024
      line = BZ2::bunzip2(BZ2::bzip2(test))
      assert_equal(test, line)

      assert(BZ2::bzip2("aaaaaaaaa".taint).tainted?)
      assert(BZ2::bunzip2(BZ2::bzip2("aaaaaaaaa".taint)).tainted?)
      assert(!BZ2::bzip2("aaaaaaaaa").tainted?)
      assert(!BZ2::bunzip2(BZ2::bzip2("aaaaaaaaa")).tainted?)

   end

   def test_zzz
      File.unlink($file)
   end
end

if defined?(RUNIT)
   RUNIT::CUI::TestRunner.run(TestWriter.suite)
end


