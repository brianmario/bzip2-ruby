# -*- encoding: utf-8 -*-
$:.push File.expand_path('../lib', __FILE__)
require 'bzip2/version'

Gem::Specification.new do |s|
  s.name     = 'bzip2-ruby'
  s.version  = Bzip2::VERSION
  s.platform = Gem::Platform::RUBY
  s.authors  = ['Guy Decoux', 'Brian Lopez']
  s.email    = ['seniorlopez@gmail.com']
  s.homepage = 'http://github.com/brianmario/bzip2-ruby'
  s.summary  = 'Ruby C bindings to libbzip2.'

  s.extensions    = ['ext/extconf.rb']
  s.files         = `git ls-files`.split("\n")
  s.test_files    = `git ls-files -- spec/*`.split("\n")
  s.require_paths = ['lib', 'ext']
end
