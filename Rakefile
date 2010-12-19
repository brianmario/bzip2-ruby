require 'bundler'
Bundler::GemHelper.install_tasks

desc "Build the C extension"
task :build_extensions do
  Dir.chdir File.expand_path('../ext', __FILE__) do
    system 'make distclean' if File.exists?('Makefile')
    system 'ruby extconf.rb && make'
  end
end

require 'rspec/core/rake_task'

RSpec::Core::RakeTask.new(:spec)
