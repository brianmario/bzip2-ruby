# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = %q{bzip2-ruby}
  s.version = "0.2.4"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Guy Decoux", "Brian Lopez"]
  s.date = %q{2009-05-02}
  s.email = %q{seniorlopez@gmail.com}
  s.extensions = ["ext/extconf.rb"]
  s.extra_rdoc_files = [
    "CHANGELOG.rdoc",
    "README.rdoc"
  ]
  s.files = [
    "CHANGELOG.rdoc",
    "README.rdoc",
    "Rakefile",
    "VERSION.yml",
    "bzip2-ruby.gemspec",
    "ext/bz2.c",
    "ext/extconf.rb",
    "spec/reader_spec.rb",
    "spec/spec_helper.rb",
    "spec/writer_spec.rb",
    "tasks/extconf.rake",
    "tasks/extconf/bz2.rake"
  ]
  s.has_rdoc = true
  s.homepage = %q{http://github.com/brianmario/bzip2-ruby}
  s.rdoc_options = ["--charset=UTF-8"]
  s.require_paths = ["ext"]
  s.rubygems_version = %q{1.3.2}
  s.summary = %q{Ruby C bindings to libbzip2.}
  s.test_files = [
    "spec/reader_spec.rb",
    "spec/spec_helper.rb",
    "spec/writer_spec.rb"
  ]

  if s.respond_to? :specification_version then
    current_version = Gem::Specification::CURRENT_SPECIFICATION_VERSION
    s.specification_version = 3

    if Gem::Version.new(Gem::RubyGemsVersion) >= Gem::Version.new('1.2.0') then
    else
    end
  else
  end
end
