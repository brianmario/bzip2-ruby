require 'mkmf'

$stat_lib = if CONFIG.key?("LIBRUBYARG_STATIC")
	       $LDFLAGS += " -L#{CONFIG['libdir']}"
	       CONFIG["LIBRUBYARG_STATIC"]
	    else
	       "-lruby"
	    end

if prefix = with_config("bz2-prefix")
   $CFLAGS += " -I#{prefix}/include"
   $LDFLAGS += " -L#{prefix}/lib"
end

if incdir = with_config("bz2-include-dir")
   $CFLAGS += " -I#{incdir}" 
end

if libdir = with_config("bz2-lib-dir")
   $LDFLAGS += " -L#{libdir}" 
end

if !have_library('bz2', 'BZ2_bzWriteOpen')
   raise "libz2 not found"
end

if enable_config("shared", true)
   $static = nil
end

create_makefile('bz2')

begin
   make = open("Makefile", "a")
   make.print <<-EOF

unknown: $(DLLIB)
\t@echo "main() {}" > /tmp/a.c
\t$(CC) -static /tmp/a.c $(OBJS) $(CPPFLAGS) $(DLDFLAGS) #$stat_lib #{CONFIG["LIBS"]} $(LIBS) $(LOCAL_LIBS)
\t@-rm /tmp/a.c a.out

%.html: %.rd
\trd2 $< > ${<:%.rd=%.html}

   EOF
   make.print "HTML = bz2.html"
   docs = Dir['docs/*.rd']
   docs.each {|x| make.print " \\\n\t#{x.sub(/\.rd$/, '.html')}" }
   make.print "\n\nRDOC = bz2.rd"
   docs.each {|x| make.print " \\\n\t#{x}" }
   make.puts
   make.print <<-EOF

rdoc: docs/doc/index.html

docs/doc/index.html: $(RDOC)
\t@-(cd docs; $(RUBY) b.rb bz2; rdoc bz2.rb)

rd2: html

html: $(HTML)

test: $(DLLIB)
   EOF
   Dir.foreach('tests') do |x|
      next if /^\./ =~ x || /(_\.rb|~)$/ =~ x
      next if FileTest.directory?(x)
      make.print "\t$(RUBY) tests/#{x}\n"
   end
ensure
   make.close
end
