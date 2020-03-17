require "spec_helper"

describe DataView do

  let(:buffer_data) { [1, 20, 255, 4, 32, 27, 175, 88, 99, 192, 32, 12, 0, 49] }
  let(:buffer) { ArrayBuffer.new(buffer_data.length) }

  before do
    buffer_data.each_with_index do |v, idx|
      buffer[idx] = v
    end
  end

  describe "bit operators" do
    let(:length) { 8 }
    let(:offset) { 2 }
    let(:dv) { described_class.new(buffer, offset, length) }

    it "has all correct bits" do
      (0...length*8).each do |index|
        byte_idx = index / 8 + offset
        bit_idx = index % 8

        bit_val = dv.getBit(index)
        expected_bit_val = (buffer_data[byte_idx] & (1 << bit_idx)) > 0 ? 1 : 0
        expect(bit_val).to eq(expected_bit_val)
      end
    end
  end

  describe "get operators" do
    let(:length) { 12 }
    let(:offset) { 1 }
    let(:endianess) { :big }
    let(:dv) { described_class.new(buffer, offset, length, endianess: endianess) }

    describe "getU8" do
      let(:length) { buffer_data.length }
      let(:dv) { described_class.new(buffer, 1) }

      it "has all correct bytes" do
        (1...length).each do |index|
          expect(dv.getU8(index - 1)).to eq(buffer_data[index])
        end
      end
    end

    describe "getU16 big endian" do
      it "returns correct values" do
        expect(dv.getU16(1)).to eq(65284) # (255 << 8) | 4
      end
    end

    describe "getU16 little endian" do
      let(:endianess) { :little }
      it "returns correct values" do
        expect(dv.getU16(1)).to eq(1279) # (4 << 8) | 255
      end
    end

    describe "getU24 big endian" do
      it "returns correct values" do
        expect(dv.getU24(1)).to eq(16712736) # (255 << 16) | (4 << 8) | 32
      end
    end

    describe "getU24 little endian" do
      let(:endianess) { :little }
      it "returns correct values" do
        expect(dv.getU24(1)).to eq(2098431) # (32 << 16) | (4 << 8) | 255
      end
    end

    describe "getU32 big endian" do
      it "returns correct values" do
        expect(dv.getU32(1)).to eq(4278460443) # (255 << 24) | (4 << 16) | (32 << 8) | 27
      end
    end

    describe "getU32 little endian" do
      let(:endianess) { :little }
      it "returns correct values" do
        expect(dv.getU32(1)).to eq(455083263) # (27 << 24) | (32 << 16) | (4 << 8) | 255
      end
    end
  end

  describe "set operators" do
    let(:length) { 12 }
    let(:offset) { 1 }
    let(:dv) { described_class.new(buffer, offset, length, endianess: endianess) }

    shared_examples "an unsigned setter" do
      let(:index) { 1 }
      it "sets the correct value" do
        dv.public_send(setter_name, index, value)
        result_value = dv.public_send(getter_name, index)
        expect(result_value).to eq(value)
      end

      it "does not change bytes before the affected area" do
        dv.public_send(setter_name, index, value)
        (0...(index+offset)).each do |i|
          expect(buffer[i]).to eq(buffer_data[i])
        end
      end

      it "does not change bytes after the affected area" do
        dv.public_send(setter_name, index, value)
        ((index+offset+affected_bytes)...buffer_data.length).each do |i|
          expect(buffer[i]).to eq(buffer_data[i])
        end
      end

      it "matches the expected byte sequence" do
        dv.public_send(setter_name, index, value)
        affected_bytes.times do |i|
          expect(buffer[index+offset+i]).to eq(expected_bytes[i])
        end
      end

      it "truncates to maximum value on overflow" do
        dv.public_send(setter_name, index, max_value + 1)
        result_value = dv.public_send(getter_name, index)
        expect(result_value).to eq(max_value)
        affected_bytes.times do |i|
          expect(buffer[index+offset+i]).to eq(0xFF)
        end
      end

      it "truncates to zero on underflow" do
        dv.public_send(setter_name, index, -1)
        result_value = dv.public_send(getter_name, index)
        expect(result_value).to eq(0)
        affected_bytes.times do |i|
          expect(buffer[index+offset+i]).to eq(0)
        end
      end
    end

    describe "setU8" do
      let(:setter_name) { :setU8 }
      let(:getter_name) { :getU8 }
      let(:affected_bytes) { 1 }
      let(:value) { 83 }
      let(:expected_bytes) { [83] }
      let(:max_value) { 0xFF }
      let(:endianess) { :big }
      it_behaves_like "an unsigned setter"
    end

    describe "setU16" do
      let(:setter_name) { :setU16 }
      let(:getter_name) { :getU16 }
      let(:affected_bytes) { 2 }
      let(:max_value) { 0xFFFF }
      let(:value) { 12391 }

      context "little endian" do
        let(:endianess) { :little }
        let(:expected_bytes) { [103, 48] }
        it_behaves_like "an unsigned setter"
      end

      context "big endian" do
        let(:endianess) { :big }
        let(:expected_bytes) { [48, 103] }
        it_behaves_like "an unsigned setter"
      end
    end

    describe "setU24" do
      let(:setter_name) { :setU24 }
      let(:getter_name) { :getU24 }
      let(:affected_bytes) { 3 }
      let(:max_value) { 0xFFFFFF }
      let(:value) { 893955 }

      context "little endian" do
        let(:endianess) { :little }
        let(:expected_bytes) { [3, 164, 13] }
        it_behaves_like "an unsigned setter"
      end

      context "big endian" do
        let(:endianess) { :big }
        let(:expected_bytes) { [13, 164, 3] }
        it_behaves_like "an unsigned setter"
      end
    end

    describe "setU32" do
      let(:setter_name) { :setU32 }
      let(:getter_name) { :getU32 }
      let(:affected_bytes) { 4 }
      let(:max_value) { 0xFFFFFFFF }
      let(:value) { 3384613905 }

      context "little endian" do
        let(:endianess) { :little }
        let(:expected_bytes) { [17, 28, 189, 201] }
        it_behaves_like "an unsigned setter"
      end

      context "big endian" do
        let(:endianess) { :big }
        let(:expected_bytes) { [201, 189, 28, 17] }
        it_behaves_like "an unsigned setter"
      end
    end
  end

  shared_examples "offset out of bounds" do
    context "when offset is greater than the underlying buffer" do
      let(:offset) { 1000 }
      it "accepts the value" do
        expect(subject.offset).to eq(1000)
      end
    end

    context "when calculated offset is negative" do
      let(:offset) { -1000 }
      it "raises" do
        expect { subject }.to raise_error(ArgumentError,
        /calculated offset is negative: -[0-9]+/)
      end
    end

    context "when offset is negative" do
      let(:offset) { -3 }
      it "has the correct value" do
        expect(subject.offset).to eq(buffer_data.length - 3)
      end
    end
  end

  shared_examples "length out of bounds" do
    context "when size is greater than the underlying buffer" do
      let(:length) { 1000 }
      it "accepts the value" do
        expect(subject.size).to eq(1000)
      end
    end

    context "when calculated size is negative" do
      let(:length) { -1000 }
      it "raises" do
        expect { subject }.to raise_error(ArgumentError,
          /calculated size is negative: -[0-9]+/)
      end
    end

    context "when size is negative" do
      let(:length) { -3 }
      it "has the correct value" do
        expect(subject.size).to eq(buffer_data.length - 3)
      end
    end
  end

  context "basic specs" do
    let(:length) { 8 }
    let(:offset) { 2 }
    let(:dv) { described_class.new(buffer, offset, length) }

    describe "initialize" do
      subject { dv }

      it_behaves_like "length out of bounds"
      it_behaves_like "offset out of bounds"
    end

    describe "length" do
      subject { dv.length }
      it { is_expected.to eq(length) }
    end

    describe "length=" do
      context "within reasonable bounds" do
        before do
          dv.length = 1
        end

        subject { dv.length }
        it { is_expected.to eq(1) }
      end

      it_behaves_like "length out of bounds" do
        let(:dv) { described_class.new(buffer, 2, 3) }
        subject do
          dv.length = length
          dv
        end
      end
    end

    describe "offset" do
      subject { dv.offset }
      it { is_expected.to be(offset) }
    end

    describe "offset=" do
      context "within reasonable bounds" do
        before do
          dv.offset = 3
        end

        subject { dv.offset }
        it { is_expected.to be(3) }
      end

      it_behaves_like "offset out of bounds" do
        let(:dv) { described_class.new(buffer, 2, 3) }
        subject do
          dv.offset = offset
          dv
        end
      end
    end

    describe "endianess" do
      it "is big endian" do
        dv = described_class.new(buffer, endianess: :big)
        expect(dv.endianess).to be(:big)
      end

      it "is little endian" do
        dv = described_class.new(buffer, endianess: :little)
        expect(dv.endianess).to be(:little)
      end
    end

    describe "to_a" do
      it "converts to array" do
        expect(dv.to_a).to eq(buffer_data[2...10])
      end
    end
  end
end
