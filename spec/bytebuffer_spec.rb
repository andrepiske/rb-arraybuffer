require "spec_helper"

describe LLC::ByteBuffer do
  let(:buffer_data) { Random.bytes(16).split('').map(&:ord) }
  let(:buffer) { described_class.new(buffer_data.length) }

  describe "initialization" do
    it "initializes all data with zeroes" do
      (0...buffer_data.length).each do |i|
        expect(buffer[i]).to eq(0)
      end
    end
  end

  private

  def set_data!
    buffer_data.each_with_index do |v, idx|
      buffer[idx] = v
    end
  end
end
