#!/usr/bin/ruby

def yield_or_not(primary)
   text = primary.sub(/\{\s*\|([^|]+)\|[^}]*\}/, '')
   if text != primary
      "def #{text}yield #$1\nend"
   else
      "def #{text}end"
   end
end

def normalize(text)
   norm = text.gsub(/\(\(\|([^|]+)\|\)\)/, '<em>\\1</em>')
   norm.gsub!(/\(\(\{/, '<tt>')
   norm.gsub!(/\}\)\)/, '</tt>')
   norm.gsub!(/\(\(<([^|>]+)[^>]*>\)\)/, '<em>\\1</em>')
   norm.gsub!(/^\s*:\s/, ' * ')
   norm
end

def intern_def(text, names, fout)
   fout.puts "##{normalize(text.join('#'))}" 
   fout.puts yield_or_not(names[0])
   if names.size > 1
      n = names[0].chomp.sub(/\(.*/, '')
      names[1 .. -1].each do |na|
	 nd = na.chomp.sub(/\(.*/, '')
	 if nd != n
	    fout.puts "#same than <em>#{n}</em>"
	    fout.puts yield_or_not(na)
	 end
      end
   end
end

def output_def(text, names, keywords, fout)
   if ! names.empty?
      keywords.each do |k|
	 fout.puts k
	 intern_def(text, names, fout)
	 fout.puts "end" if k != ""
      end
   end
end

def loop_file(file, fout)
   text, keywords, names = [], [""], []
   comment = false
   rep, indent, vide = '', -1, nil
   IO.foreach(file) do |line|
      if /^#\^/ =~ line
	 comment = ! comment
	 next
      end
      if comment
	 fout.puts "# #{normalize(line)}"
	 next
      end
      case line
      when /^\s*$/
	 vide = true
	 text.push line
      when /^#\^/
	 comment = ! comment
      when /^##/
	 line[0] = ?\s
	 fout.puts line
      when /^#\s*(.+?)\s*$/
	 keyword = $1
	 output_def(text, names, keywords, fout)
	 text, names = [], []
	 keywords = keyword.split(/\s*##\s*/)
	 if keywords.size == 1
	    fout.puts keywords[0]
	    keywords = [""]
	 end
      when /^#/
      when /^---/
	 name = $'
	 if vide
	    output_def(text, names, keywords, fout)
	    text, names = [], []
	    rep, indent, vide = '', -1, false
	 end
	 names.push name
      else
	 vide = false
	 if line.sub!(/^(\s*): /, '* ')
	    indent += ($1 <=> rep)
	    rep = $1
	 else
	    line.sub!(/^#{rep}/, '')
	 end
	 if indent >= 0
	    line = ('  ' * indent) + line
	 else
	    line.sub!(/\A\s*/, '')
	 end
	 text.push line
      end
   end
end

File.open("#{ARGV[0]}.rb", "w") do |fout|
   loop_file("../#{ARGV[0]}.rd", fout)
   Dir['*.rd'].each do |file|
      loop_file(file, fout)
   end
end
