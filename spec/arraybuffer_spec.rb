require "spec_helper"

describe ArrayBuffer do
  let(:buffer_bytes) { Random.bytes(16) }
  let(:buffer_data) { buffer_bytes.split('').map(&:ord) }
  let(:buffer) { described_class.new(buffer_data.length) }

  describe "initialization" do
    it "initializes all data with zeroes" do
      (0...buffer_data.length).each do |i|
        expect(buffer[i]).to eq(0)
      end
    end
  end

  describe "realloc" do
    before { set_data! }

    it "has the new size" do
      buffer.realloc(6)
      expect(buffer.size).to eq(6)
    end

    it "still contains the partial content" do
      buffer.realloc(6)
      expect(buffer.bytes).to eq(buffer_bytes[0...6])
    end
  end

  describe "bytes" do
    context "when buffer has data" do
      before { set_data! }

      it "converts the buffer to a string" do
        expect(buffer.bytes).to eq(buffer_bytes)
      end

      it "is ascii-8bit encoded" do
        expect(buffer.bytes.encoding).to be(Encoding.find("ASCII-8BIT"))
      end
    end

    context "when buffer is empty" do
      let(:buffer) { described_class.new(0) }

      it "returns empty string" do
        expect(buffer.bytes).to eq("")
      end

      it "is ascii-8bit encoded" do
        expect(buffer.bytes.encoding).to be(Encoding.find("ASCII-8BIT"))
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
