# frozen_string_literal: true

Gem::Specification.new do |s|
  s.name          = "arraybuffer"
  s.license       = "MIT"
  s.summary       = "An array buffer (a.k.a. byte array) implementation for" \
                  + "Ruby, implemented natively."

  s.authors       = ["André Diego Piske"]
  s.email         = ["andrepiske@gmail.com"]
  s.homepage      = "https://github.com/andrepiske/rb-arraybuffer"

  s.files         = Dir.glob([ "lib/**/*", "ext/**/*", "extconf.h" ])
  s.require_paths = ["lib"]
  s.version       = "0.0.4"

  s.metadata      = {
    "source_code_uri" => "https://github.com/andrepiske/rb-arraybuffer",
  }

  s.extensions = ["ext/arraybuffer/extconf.rb"]

  s.required_ruby_version = ">= 2.1"

  s.add_development_dependency "rspec", "~> 3.9"
end
