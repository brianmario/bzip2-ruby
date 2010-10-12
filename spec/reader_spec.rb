# encoding: UTF-8
require 'spec_helper'

describe Bzip2::Writer do
  before(:each) do
    @sample = "08: This is a line\n"
    @file = "_10lines_"
    @data = [
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

    open("|bzip2 > #{@file}", "w") do |f|
      @data.each { |l| f.puts l }
    end
  end

  after(:each) do
    File.delete(@file) if File.exists?(@file)
  end

  it "iterate over each line of the file via the foreach method" do
    lines = []
    Bzip2::Reader.foreach(@file){ |line| lines << line }
    lines.should == @data

    lines.clear
    Bzip2::Reader.foreach(@file, nil) do |file|
      file.split(/\n/).each{ |line| lines << line + "\n" }
    end
    lines.should == @data

    count = 0
    Bzip2::Reader.foreach(@file, ' ') do |thing|
      count += 1
    end
    count.should == 41
  end

  it "returns an array of the lines read via #readlines" do
    lines = Bzip2::Reader.readlines(@file)
    lines.should == @data

    lines = Bzip2::Reader.readlines(@file, nil)
    lines.should == [@data.join]
  end

  it "track when the stream has been closed" do
    f = Bzip2::Reader.open(@file)
    f.should_not be_closed
    f.close
    f.should be_closed
  end

  shared_examples_for 'a line iterator' do |method|
    it "iterates over the lines when using #each" do
      Bzip2::Reader.open(@file) do |file|
        list = []
        file.send(method){ |l| list << l }
        list.should == @data
      end

      Bzip2::Reader.open(@file) do |file|
        file.send(method, nil) do |contents|
          contents.should == @data.join
        end
      end

      count = 0
      Bzip2::Reader.open(@file) do |file|
        file.send(method, ' ') do |thing|
          count += 1
        end
      end
      41.should == count
    end
  end

  it_should_behave_like 'a line iterator', :each
  it_should_behave_like 'a line iterator', :each_line

  it "iterates over the decompressed bytes via #each_byte" do
    bytes = @data.join.bytes.to_a

    Bzip2::Reader.open(@file) do |file|
      file.each_byte do |b|
        b.should == bytes.shift
      end
    end

    bytes.size.should == 0
  end

  it "keeps track of when eof has been reached" do
    Bzip2::Reader.open(@file) do |file|
      @data.size.times do
        file.should_not be_eof
        file.gets
      end

      file.should be_eof
    end
  end

  it "gets only one byte at a time via getc and doesn't raise an exception" do
    bytes = @data.join.bytes.to_a

    Bzip2::Reader.open(@file) do |file|
      while ch = file.getc
        ch.should == bytes.shift
      end

      file.getc.should be_nil
    end

    bytes.size.should == 0
  end

  it "reads an entire line via gets" do
    Bzip2::Reader.open(@file) do |file|
      lines = []
      while line = file.gets
        lines << line
      end
      lines.should == @data

      file.gets.should be_nil
    end

    Bzip2::Reader.open(@file) do |file|
      lines = []
      while line = file.gets("line\n")
        lines << line
      end
      lines.should == @data

      file.gets.should be_nil
    end

    lines = ''
    Bzip2::Reader.open(@file) do |file|
      while contents = file.gets(nil)
        lines << contents
      end
    end
    lines.should == @data.join

    count = 0
    Bzip2::Reader.open(@file) do |file|
      count += 1 while file.gets(' ')
    end
    41.should == count
  end

  it "reads the entire file or a specified length when using #read" do
    Bzip2::Reader.open(@file) do |file|
      file.read.should == @data.join
    end

    Bzip2::Reader.open(@file) do |file|
      file.read(12).should == "00: This is "
      file.read(12).should == "a line\n01: T"
    end
  end

  it "reads one character and returns the byte value of the character read" do
    count = 0
    data = @data.join
    Bzip2::Reader.open(@file) do |file|
      @data.join.bytes do |byte|
        file.readchar.should == byte
      end

      lambda { file.readchar }.should raise_error(Bzip2::EOZError)
    end
  end

  it "reads one line at a time and raises and exception when no more" do
    count = 0
    Bzip2::Reader.open(@file) do |file|
      lines = []
      @data.size.times do |count|
        lines << file.readline
      end

      lines.should == @data
      lambda { file.readline }.should raise_error(Bzip2::EOZError)
    end

    Bzip2::Reader.open(@file) do |file|
      file.readline(nil).should == @data.join

      lambda { file.readline }.should raise_error(Bzip2::EOZError)
    end

    Bzip2::Reader.open(@file) do |file|
      41.times { |count| file.readline(' ') }
      lambda { file.readline }.should raise_error(Bzip2::EOZError)
    end
  end

  it "returns an array of lines in the file" do
    Bzip2::Reader.open(@file) do |file|
      file.readlines.should == @data
    end

    Bzip2::Reader.open(@file) do |file|
      file.readlines(nil).should == [@data.join]
    end
  end

  it "rewinds the stream when #ungetc is called and returns that byte next" do
    Bzip2::Reader.open(@file) do |file|
      '0'.bytes.first.should == file.getc
      '0'.bytes.first.should == file.getc
      ':'.bytes.first.should == file.getc
      ' '.bytes.first.should == file.getc

      file.ungetc(':'.bytes.first).should be_nil
      ':'.bytes.first.should == file.getc

      file.read

      file.ungetc('A'.bytes.first).should be_nil
      'A'.bytes.first.should == file.getc
    end
  end

  it "rewinds the stream when #ungets is called" do
    Bzip2::Reader.open(@file) do |file|
      @data[0].should == file.gets
      1.should == file.lineno
      file.ungets(@data[0]).should be_nil
      @data[0].should == file.gets
    end
  end

  it "reads entire lines via readline and throws an exception when there is" do
    string = File.read(@file)
    file = Bzip2::Reader.new(string)
    lines = []
    @data.size.times do |count|
      lines << file.readline
    end
    lines.should == @data
    lambda { file.readline }.should raise_error(Bzip2::EOZError)
    file.close

    file = Bzip2::Reader.new(string)
    file.readline(nil).should == @data.join
    lambda { file.readline }.should raise_error(Bzip2::EOZError)
    file.close
  end
end
