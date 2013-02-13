# Ruby C bindings to libbzip2

## Installation

First make sure you’ve got Gemcutter in your sources list:

`gem sources -a http://gemcutter.org`

Then go ahead and install it as usual:

`sudo gem install bzip2-ruby`

You may need to specify:

`--with-bz2-dir=<include file directory for libbzip2>`

Or in a Gemfile

`gem 'bzip2-ruby'`

## Usage

The full documentation is hosted on {rdoc.info}[http://rdoc.info/github/brianmario/bzip2-ruby/master/frames].

Here's a quick overview, hower:

``` ruby
require 'bzip2'

# Quick shortcuts
data = Bzip2.compress 'string'
Bzip2.uncompress data

# Creating a bz2 compressed file
writer = Bzip2::Writer.new File.open('file')
writer << 'data1'
writer.puts 'data2'
writer.print 'data3'
writer.printf '%s', 'data4'
writer.close

Bzip2::Writer.open('file'){ |f| f << data }

# Reading a bz2 compressed file
reader = Bzip2::Reader.new File.open('file')
reader.gets # => "data1data2\n"
reader.read # => 'data3data4'

reader.readline # => raises Bzip2::EOZError

Bzip2::Reader.open('file'){ |f| puts f.read }
```

## Copying

```
This extension module is copyrighted free software by Guy Decoux
You can redistribute it and/or modify it under the same term as Ruby.
Guy Decoux <ts@moulon.inra.fr>
```

## Modifications from origin version

* Switch to Jeweler
* Renamed BZ2 module/namespace to Bzip2
* Renamed compiled binary from "bz2" to "bzip2"
* Renamed gem from "bz2" to "bzip2-ruby"
* Converted original tests to rspec
* 1.9 compatibility
