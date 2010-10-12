# encoding: UTF-8
require 'spec_helper'

describe Bzip2::Writer do
  class Dummy
    def to_s
      "dummy"
    end
  end

  before(:all) do
    @file = "_10lines_"
  end

  after(:all) do
    File.unlink(@file)
  end

  it "should test_f_LSHIFT" do
    Bzip2::Writer.open(@file, "w") do |file|
      file << 1 << "\n" << Dummy.new << "\n" << "cat\n"
    end
    expected = [ "1\n", "dummy\n", "cat\n"]
    Bzip2::Reader.foreach(@file) do |line|
      expected.shift.should == line
    end
    [].should == expected
  end

  it "should test_f_error" do
    io = File.new(@file, "w")
    bz2 = Bzip2::Writer.new(io)
    bz2 << 1 << "\n" << Dummy.new << "\n" << "cat\n"
    bz = Bzip2::Reader.new(@file)
    lambda { bz.gets }.should raise_error(Bzip2::Error)
    bz = Bzip2::Reader.open(@file)
    lambda { bz.gets }.should raise_error(Bzip2::Error)
    io.close
    lambda { Bzip2::Reader.new(io) }.should raise_error(IOError)
  end

  it "should test_f_gets_para" do
    Bzip2::Writer.open(@file) do |file|
      file.print "foo\n"*4096, "\n"*4096, "bar"*4096, "\n"*4096, "zot\n"*1024
    end
    Bzip2::Reader.open(@file) do |file|
      ("foo\n"*4096+"\n").should == file.gets("")
      ("bar"*4096+"\n\n").should == file.gets("")
      ("zot\n"*1024).should == file.gets("")
    end
  end

  it "should test_f_print" do
    Bzip2::Writer.open(@file) do |file|
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

    Bzip2::Reader.open(@file) do |file|
      content = file.gets(nil)
      "hello12wombat\n3,4:5,6:\n".should == content
    end
  end

  it "should test_f_putc" do
    Bzip2::Writer.open(@file, "wb") do |file|
      file.putc "A"
      0.upto(255) { |ch| file.putc ch }
    end

    Bzip2::Reader.open(@file, "rb") do |file|
      ?A.should == file.getc
      0.upto(255) { |ch| ch.should == file.getc }
    end
  end

  it "should test_f_puts" do
    Bzip2::Writer.open(@file, "w") do |file|
      file.puts "line 1", "line 2"
      file.puts [ Dummy.new, 4 ]
    end

    Bzip2::Reader.open(@file) do |file|
      "line 1\n".should == file.gets
      "line 2\n".should == file.gets
      "dummy\n".should == file.gets
      "4\n".should == file.gets
    end
  end

  it "should test_f_write" do
    Bzip2::Writer.open(@file, "w") do |file|
      10.should == file.write('*' * 10)
      5.should == file.write('!' * 5)
      0.should == file.write('')
      1.should == file.write(1)
      3.should == file.write(2.30000)
      1.should == file.write("\n")
    end

    Bzip2::Reader.open(@file) do |file|
      "**********!!!!!12.3\n".should == file.gets
    end
  end

  it "should test_s_string" do
    file = Bzip2::Writer.new
    10.should == file.write('*' * 10)
    5.should == file.write('!' * 5)
    0.should == file.write('')
    1.should == file.write(1)
    3.should == file.write(2.30000)
    1.should == file.write("\n")
    line = Bzip2::bunzip2(file.flush)
    "**********!!!!!12.3\n".should == line

    line = Bzip2::bunzip2(Bzip2::bzip2("**********!!!!!12.3\n"))
    "**********!!!!!12.3\n".should == line

    test = "foo\n"*4096 + "\n"*4096 + "bar"*4096 + "\n"*4096 + "zot\n"*1024
    line = Bzip2::bunzip2(Bzip2::bzip2(test))
    test.should == line
  end
end
