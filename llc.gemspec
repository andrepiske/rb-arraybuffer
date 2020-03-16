# frozen_string_literal: true

Gem::Specification.new do |spec|
  spec.authors       = ["André Diego Piske"]
  spec.email         = ["andrepiske@gmail.com"]
  spec.homepage      = "https://github.com/andrepiske/llc"
  spec.license       = "MIT"
  spec.summary       = "Low level byte operators and buffers"

  spec.files         = Dir.glob([ "lib/**/*", "ext/**/*", "extconf.h" ])
  spec.name          = "llc"
  spec.require_paths = ["lib"]
  spec.version       = "0.0.2"

  spec.required_ruby_version = ">= 2.1"

  spec.extensions = ["ext/llc/extconf.rb"]

  spec.add_development_dependency "rspec", "~> 3.9"
end
