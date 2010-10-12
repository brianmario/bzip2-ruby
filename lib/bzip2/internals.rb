# This file is mostly here for documentation purposes, do not require this

module Bzip2
  class << self
    alias :bzip2 :compress
    alias :bunzip2 :uncompress
    alias :decompress :uncompress
  end

  # @private
  class InternalStr
  end
end
