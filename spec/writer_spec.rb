# encoding: UTF-8
require 'spec_helper'

describe Bzip2::Writer do
  class Dummy
    def to_s
      "dummy"
    end
  end

  before(:each) do
    @file = File.expand_path('../_10lines_', __FILE__)
  end

  after(:each) do
    File.delete(@file) if File.exists?(@file)
  end

  it "performs like IO#<< when using the #<< method" do
    Bzip2::Writer.open(@file, "w") do |file|
      file << 1 << "\n" << Dummy.new << "\n" << "cat\n"
    end
    expected = [ "1\n", "dummy\n", "cat\n"]
    actual   = []
    Bzip2::Reader.foreach(@file){ |line| actual.push line }
    actual.should == expected
  end

  it "doesn't immediately flush the data when written to" do
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

  it "behaves the same as IO#print when using #print" do
    Bzip2::Writer.open(@file) do |file|
      file.print "foo\n" * 4096, "\n" * 4096,
                 "bar" * 4096, "\n" * 4096, "zot\n" * 1024
    end

    Bzip2::Reader.open(@file) do |file|
      file.gets('').should == "foo\n" * 4096 + "\n"
      file.gets('').should == "bar" * 4096 + "\n\n"
      file.gets('').should == "zot\n" * 1024
    end
  end

  it "respects specific global variables like IO#print does via #print" do
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
      file.gets(nil).should == "hello12wombat\n3,4:5,6:\n"
    end
  end

  it "only writes one byte via the #putc method" do
    Bzip2::Writer.open(@file, "wb") do |file|
      file.putc "A"
      0.upto(255) { |ch| file.putc ch }
    end

    Bzip2::Reader.open(@file, "rb") do |file|
      file.getc.should == 'A'.bytes.first
      0.upto(255) { |ch| file.getc.should == ch }
    end
  end

  it "behaves the same as IO#puts when using #puts" do
    Bzip2::Writer.open(@file, "w") do |file|
      file.puts "line 1", "line 2"
      file.puts [ Dummy.new, 4 ]
    end

    Bzip2::Reader.open(@file) do |file|
      file.gets.should == "line 1\n"
      file.gets.should == "line 2\n"
      file.gets.should == "dummy\n"
      file.gets.should == "4\n"
    end
  end

  it "writes data successfully to a file and returns the length of the data" do
    Bzip2::Writer.open(@file, "w") do |file|
      file.write('*' * 10).should == 10
      file.write('!' * 5).should == 5
      file.write('').should == 0
      file.write(1).should == 1
      file.write(2.30000).should == 3
      file.write("\n").should == 1
    end

    Bzip2::Reader.open(@file) do |file|
      file.gets.should == "**********!!!!!12.3\n"
    end
  end

  it "returns the compressed data when no constructor argument is specified" do
    file = Bzip2::Writer.new
    file << ('*' * 10) << ('!' * 5) << '' << 1 << 2.3000 << "\n"
    Bzip2::bunzip2(file.flush).should == "**********!!!!!12.3\n"
  end

  it "compresses data via the #bzip2 shortcut" do
    data = ["**********!!!!!12.3\n"]
    data << "foo\n"*4096 + "\n"*4096 + "bar"*4096 + "\n"*4096 + "zot\n"*1024

    data.each do |test|
      Bzip2::bunzip2(Bzip2::bzip2(test)).should == test
    end
  end
end
