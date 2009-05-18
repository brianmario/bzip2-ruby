# encoding: UTF-8
require File.expand_path(File.dirname(__FILE__) + '/spec_helper.rb')

describe "Bzip2::Writer" do
  before(:all) do
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
  
  after(:all) do
    File.unlink(@file)
  end

  it "should test_f_s_foreach" do
    count = 0
    Bzip2::Reader.foreach(@file) do |line|
      num = line[0..1].to_i
      count.should == num
      count += 1
    end
    @data.size.should == count

    count = 0
    Bzip2::Reader.foreach(@file, nil) do |file|
      file.split(/\n/).each do |line|
        num = line[0..1].to_i
        count.should == num
        count += 1
      end
    end
    @data.size.should == count

    count = 0
    Bzip2::Reader.foreach(@file, ' ') do |thing|
      count += 1
    end
    41.should == count
  end
  
  it "should test_f_s_readlines" do
    lines = Bzip2::Reader.readlines(@file)
    @data.size.should == lines.size

    lines = Bzip2::Reader.readlines(@file, nil)
    1.should == lines.size
    (@sample.length * @data.size).should == lines[0].size
  end

  it "should test_f_closed?" do
    f = Bzip2::Reader.open(@file)
    f.closed?.should be_false
    f.close
    f.closed?.should be_true
  end

  it "should test_f_each" do
    count = 0
    Bzip2::Reader.open(@file) do |file|
      file.each do |line|
        num = line[0..1].to_i
        count.should == num
        count += 1
      end
      @data.size.should == count
    end

    count = 0
    Bzip2::Reader.open(@file) do |file|
      file.each(nil) do |contents|
        contents.split(/\n/).each do |line|
          num = line[0..1].to_i
          count.should == num
          count += 1
        end
      end
    end
    @data.size.should == count

    count = 0
    Bzip2::Reader.open(@file) do |file|
      file.each(' ') do |thing|
        count += 1
      end
    end
    41.should == count
  end

  it "should test_f_each_byte" do
    count = 0
    data = @data.join

    Bzip2::Reader.open(@file) do |file|
      file.each_byte do |b|
        data.getbyte(count).should == b
        count += 1
      end
    end
    (@sample.length * @data.size).should == count
  end

  it "should test_f_each_line" do
    count = 0
    Bzip2::Reader.open(@file) do |file|
      file.each_line do |line|
        num = line[0..1].to_i
        count.should == num
        count += 1
      end
      @data.size.should == count
    end

    count = 0
    Bzip2::Reader.open(@file) do |file|
      file.each_line(nil) do |contents|
        contents.split(/\n/).each do |line|
          num = line[0..1].to_i
          count.should == num
          count += 1
        end
      end
    end
    @data.size.should == count

    count = 0
    Bzip2::Reader.open(@file) do |file|
      file.each_line(' ') do |thing|
        count += 1
      end
    end
    41.should == count
  end

  it "should test_f_eof" do
    Bzip2::Reader.open(@file) do |file|
      @data.size.times do
        (!file.eof).should be_true
        (!file.eof?).should be_true
        file.gets
      end
      file.eof.should be_true
      file.eof?.should be_true
    end
  end

  it "should test_f_getc" do
    count = 0
    data = @data.join

    Bzip2::Reader.open(@file) do |file|
      while (ch = file.getc)
        data.getbyte(count).should == ch
        count += 1
      end
      file.getc.should be_nil
    end
    (@sample.length * @data.size).should == count
  end

  it "should test_f_gets" do
    count = 0
    Bzip2::Reader.open(@file) do |file|
      while (line = file.gets)
        num = line[0..1].to_i
        count.should == num
        count += 1
      end
      file.gets.should be_nil
      @data.size.should == count
    end

    count = 0
    Bzip2::Reader.open(@file) do |file|
      while (line = file.gets("line\n"))
        @data[count].should == line
        num = line[0..1].to_i
        count.should == num
        count += 1
      end
      file.gets.should be_nil
      @data.size.should == count
    end

    count = 0
    Bzip2::Reader.open(@file) do |file|
      while (contents = file.gets(nil))
        contents.split(/\n/).each do |line|
          num = line[0..1].to_i
          count.should == num
          count += 1
        end
      end
    end
    @data.size.should == count

    count = 0
    Bzip2::Reader.open(@file) do |file|
      while (thing = file.gets(' '))
        count += 1
      end
    end
    41.should == count
  end

  it "should test_f_read" do
    Bzip2::Reader.open(@file) do |file|
      content = file.read
      (@sample.length * @data.size).should == content.length
      count = 0
      content.split(/\n/).each do |line|
        num = line[0..1].to_i
        count.should == num
        count += 1
      end
    end

    Bzip2::Reader.open(@file) do |file|
      "00: This is ".should == file.read(12)
      "a line\n01: T".should == file.read(12)
    end
  end

  it "should test_f_readchar" do
    count = 0
    data = @data.join
    Bzip2::Reader.open(@file) do |file|
      190.times do |count|
        ch = file.readchar
        data.getbyte(count).should == ch
        count += 1
      end
      lambda { file.readchar }.should raise_error(Bzip2::EOZError)
    end
  end

  it "should test_f_readline" do
    count = 0
    Bzip2::Reader.open(@file) do |file|
      @data.size.times do |count|
        line = file.readline
        num = line[0..1].to_i
        count.should == num
        count += 1
      end
      lambda { file.readline }.should raise_error(Bzip2::EOZError)
    end

    count = 0
    Bzip2::Reader.open(@file) do |file|
      contents = file.readline(nil)
      contents.split(/\n/).each do |line|
        num = line[0..1].to_i
        count.should == num
        count += 1
      end
      lambda { file.readline }.should raise_error(Bzip2::EOZError)
    end
    @data.size.should == count

    count = 0
    Bzip2::Reader.open(@file) do |file|
      41.times do |count|
        thing = file.readline(' ')
        count += 1
      end
      lambda { file.readline }.should raise_error(Bzip2::EOZError)
    end
  end

  it "should test_f_readlines" do
    Bzip2::Reader.open(@file) do |file|
      lines = file.readlines
      @data.size.should == lines.size
    end

    Bzip2::Reader.open(@file) do |file|
      lines = file.readlines(nil)
      1.should == lines.size
      (@sample.length * @data.size).should == lines[0].size
    end
  end

  # it "should test_f_ungetc" do
  #   Bzip2::Reader.open(@file) do |file|
  #     ?0.getbyte(0).should == file.getc
  #     ?0.getbyte(0).should == file.getc
  #     ?:.getbyte(0).should == file.getc
  #     ?\s.getbyte(0).should == file.getc
  #     file.ungetc(?:.to_i).should be_nil
  #     ?:.getbyte(0).should == file.getc
  #     1 while file.getc
  #     file.ungetc(?A).should be_nil
  #     ?A.should == file.getc
  #   end
  # end

  it "should test_f_ungets" do
    count = 0
    Bzip2::Reader.open(@file) do |file|
      @data[count].should == file.gets
      (count + 1).should == file.lineno
      file.ungets(@data[count]).should be_nil
      @data[count].should == file.gets
      count += 1
    end
  end

  it "should test_s_readline" do
    count = 0
    string = IO.readlines(@file, nil)[0]
    file = Bzip2::Reader.new(string)
    @data.size.times do |count|
      line = file.readline
      num = line[0..1].to_i
      count.should == num
      count += 1
    end
    lambda { file.readline }.should raise_error(Bzip2::EOZError)
    file.close

    count = 0
    file = Bzip2::Reader.new(string)
    contents = file.readline(nil)
    contents.split(/\n/).each do |line|
      num = line[0..1].to_i
      count.should == num
      count += 1
    end
    lambda { file.readline }.should raise_error(Bzip2::EOZError)
    @data.size.should == count
    file.close

    count = 0
    file = Bzip2::Reader.new(string)
    41.times do |count|
      thing = file.readline(' ')
      count += 1
    end
    lambda { file.readline }.should raise_error(Bzip2::EOZError)
    file.close
  end
end