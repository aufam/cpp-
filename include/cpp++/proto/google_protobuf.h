#ifndef CPPXX_PROTO_GOOGLE_PROTOBUF_H
#define CPPXX_PROTO_GOOGLE_PROTOBUF_H

#include <cpp++/proto/proto.h>
#include <cpp++/serde/serialize.h>
#include <cpp++/serde/deserialize.h>
#include <cpp++/serde/error.h>
#include <array>
#include <tuple>
#include <string>
#include <vector>

#ifndef GOOGLE_PROTOBUF_IO_CODED_STREAM_H__
#    include <google/protobuf/io/coded_stream.h>
#endif

#ifndef GOOGLE_PROTOBUF_IO_ZERO_COPY_STREAM_IMPL_LITE_H__
#    include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#endif

#ifndef GOOGLE_PROTOBUF_WIRE_FORMAT_LITE_H__
#    include <google/protobuf/wire_format_lite.h>
#endif

#ifndef BOOST_PFR_HPP
#    if __has_include(<boost/pfr.hpp>)
#        include <boost/pfr.hpp>
#    endif
#endif


namespace cppxx::proto::google_protobuf {
    template <typename From>
    using Serialize = ::cppxx::serde::Serialize<google::protobuf::io::CodedOutputStream, From>;

    using Dump = ::cppxx::serde::Dump<google::protobuf::io::CodedOutputStream, std::string>;

    template <typename T>
    [[nodiscard]]
    std::string dump(const T &val);
} // namespace cppxx::proto::google_protobuf

namespace cppxx::serde {
    template <>
    struct Dump<google::protobuf::io::CodedOutputStream, std::string> {
        template <typename... Ts>
        std::string from(const std::tuple<Ts...> &tpl) const {
            std::string                              buffer;
            google::protobuf::io::StringOutputStream os(&buffer);
            google::protobuf::io::CodedOutputStream  doc(&os);

            google::protobuf::io::ArrayInputStream ais(buffer.data(), (int)buffer.size());

            tuple_for_each(tpl, [&](const auto &v, size_t) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (is_serializable_v<google::protobuf::io::CodedOutputStream, T>)
                    Serialize<google::protobuf::io::CodedOutputStream, T>{doc}.from(v);
            });

            return buffer;
        }

#ifdef BOOST_PFR_HPP
        template <typename S>
        std::enable_if_t<std::is_aggregate_v<S>, std::string> from(const S &v) const {
            auto tpl = boost::pfr::structure_tie(v);
            return from(tpl);
        }
#endif
    };

    template <>
    struct Parse<google::protobuf::io::CodedInputStream, std::string> {
        const std::string &buffer;

        template <typename... Ts>
        void into(std::tuple<Ts...> &tpl) const {
            google::protobuf::io::ArrayInputStream ais(buffer.data(), (int)buffer.size());
            google::protobuf::io::CodedInputStream doc(&ais);

            std::array<TagInfo, sizeof...(Ts)>     tis    = {};
            std::array<int, sizeof...(Ts)>         fns    = {};
            std::array<std::string, sizeof...(Ts)> raws   = {};
            std::array<uint32_t, sizeof...(Ts)>    raws32 = {};
            std::array<uint64_t, sizeof...(Ts)>    raws64 = {};

            tuple_for_each(tpl, [&](const auto &v, size_t i) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (is_tagged_v<T>) {
                    tis[i] = proto::get_tag_info(v);
                    fns[i] = proto::get_field_number(tis[i]);
                }
            });

            auto get_index = [&](int field_number) {
                size_t i = 0;
                for (; i < fns.size(); i++)
                    if (fns[i] == field_number)
                        break;
                return i;
            };

            while (!doc.ConsumedEntireMessage()) {
                uint32_t tag;
                if (!doc.ReadVarint32(&tag))
                    break;

                const uint32_t field_number = tag >> 3;
                const uint32_t wire_type    = tag & 0x07;
                const size_t   index        = get_index(field_number);

                std::string &raw   = raws[index];
                uint32_t    &raw32 = raws32[index];
                uint64_t    &raw64 = raws64[index];

                switch (wire_type) {
                case google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT:
                    doc.ReadVarint64(&raw64);
                    raw.assign(reinterpret_cast<char *>(&raw64), sizeof(raw64));
                    break;
                case google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED64:
                    doc.ReadLittleEndian64(&raw64);
                    raw.assign(reinterpret_cast<char *>(&raw64), sizeof(raw64));
                    break;
                case google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED32:
                    doc.ReadLittleEndian32(&raw32);
                    raw.assign(reinterpret_cast<char *>(&raw32), sizeof(raw32));
                    break;
                case google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED:
                    doc.ReadVarint32(&raw32);
                    doc.ReadString(&raw, (int)raw32);
                    break;
                default:
                    break;
                }
            }

            tuple_for_each(tpl, [&](auto &v, size_t i) {
                std::string raw = std::move(raws[i]);

                using T = std::decay_t<decltype(v)>;
                if constexpr (is_deserializable_v<google::protobuf::io::CodedInputStream, T>)
                    Deserialize<google::protobuf::io::CodedInputStream, T>{raw, raws32[i], raws64[i]}.into(
                        detail::get_underlying_value(v), tis[i]
                    );
            });
        }

#ifdef BOOST_PFR_HPP
        template <typename S>
        std::enable_if_t<std::is_aggregate_v<S>> into(const S &v) const {
            auto tpl = boost::pfr::structure_tie(v);
            into(tpl);
        }
#endif

        template <typename T>
        static std::pair<T, bool> read_varint(const std::string &raw, const TagInfo &t) {
            if (t.key.empty())
                return {0, false};
            if (raw.empty()) {
                if (t.skipmissing)
                    return {0, false};
                throw error(t.key, "missing field");
            }
            T v;
            if (raw.size() != sizeof(v))
                throw error(
                    t.key, "mismatch message size, expect " + std::to_string(sizeof(v)) + " got " + std::to_string(raw.size())
                );
            ::memcpy(&v, raw.data(), sizeof(v));
            return {v, true};
        }

        static std::pair<uint64_t, bool> read_fixed64(const std::string &raw, const TagInfo &t) {
            return read_varint<uint64_t>(raw, t);
        }

        static std::pair<uint32_t, bool> read_fixed32(const std::string &raw, const TagInfo &t) {
            return read_varint<uint32_t>(raw, t);
        }
    };

    // bool
    template <>
    struct Serialize<google::protobuf::io::CodedOutputStream, Tag<bool>> {
        google::protobuf::io::CodedOutputStream &doc;

        void from(const Tag<bool> &v) const {
            from(v.get_value(), proto::get_tag_info(v));
        }

        void from(bool v, const TagInfo &t) const {
            if (t.key != "" && !(t.omitempty && detail::is_empty_value(v)))
                from(v, proto::get_field_number(t));
        }

        void from(bool v, int field_number) const {
            doc.WriteTag(
                google::protobuf::internal::WireFormatLite::MakeTag(
                    field_number, google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT
                )
            );
            doc.WriteVarint32(v);
        }
    };

    template <>
    struct Deserialize<google::protobuf::io::CodedInputStream, Tag<bool>> {
        const std::string &raw;
        uint32_t           raw32 = 0;
        uint64_t           raw64 = 0;

        void into(Tag<bool> &v) const {
            into(v.get_value(), proto::get_tag_info(v));
        }

        void into(bool &v, const TagInfo &t) const {
            auto [val, ok] = Parse<google::protobuf::io::CodedInputStream, std::string>::read_fixed64(raw, t);
            if (ok)
                v = val;
        }
    };

    // uint32_t
    template <>
    struct Serialize<google::protobuf::io::CodedOutputStream, Tag<uint32_t>> {
        google::protobuf::io::CodedOutputStream &doc;

        void from(const Tag<uint32_t> &v) const {
            from(v.get_value(), proto::get_tag_info(v));
        }

        void from(uint32_t v, const TagInfo &t) const {
            if (t.key != "" && !(t.omitempty && detail::is_empty_value(v)))
                from(v, proto::get_field_number(t));
        }

        void from(uint32_t v, int field_number) const {
            doc.WriteTag(
                google::protobuf::internal::WireFormatLite::MakeTag(
                    field_number, google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT
                )
            );
            doc.WriteVarint32(v);
        }
    };

    template <>
    struct Deserialize<google::protobuf::io::CodedInputStream, Tag<uint32_t>> {
        std::string raw;

        void into(Tag<uint32_t> &v) const {
            into(v.get_value(), proto::get_tag_info(v));
        }

        void into(uint32_t &v, const TagInfo &t) const {
            if (raw.size() == sizeof(uint32_t)) {
                auto [val, ok] = Parse<google::protobuf::io::CodedInputStream, std::string>::read_fixed32(raw, t);
                if (ok)
                    v = val;
            } else {
                auto [val, ok] = Parse<google::protobuf::io::CodedInputStream, std::string>::read_fixed64(raw, t);
                if (ok)
                    v = val;
            }
        }
    };

    // int32_t
    template <>
    struct Serialize<google::protobuf::io::CodedOutputStream, Tag<int32_t>> {
        google::protobuf::io::CodedOutputStream &doc;

        void from(const Tag<int32_t> &v) const {
            from(v.get_value(), proto::get_tag_info(v));
        }

        void from(int32_t v, const TagInfo &t) const {
            Serialize<google::protobuf::io::CodedOutputStream, Tag<uint32_t>>{doc}.from(static_cast<uint32_t>(v), t);
        }

        void from(int32_t v, int field_number) const {
            Serialize<google::protobuf::io::CodedOutputStream, Tag<uint32_t>>{doc}.from(static_cast<uint32_t>(v), field_number);
        }
    };

    template <>
    struct Deserialize<google::protobuf::io::CodedInputStream, Tag<int32_t>> {
        std::string raw;

        void into(Tag<int32_t> &v) const {
            into(v.get_value(), proto::get_tag_info(v));
        }

        void into(int32_t &v, const TagInfo &t) const {
            if (raw.size() == sizeof(int32_t)) {
                auto [val, ok] = Parse<google::protobuf::io::CodedInputStream, std::string>::read_fixed32(raw, t);
                if (ok)
                    v = static_cast<int32_t>(val);
            } else {
                auto [val, ok] = Parse<google::protobuf::io::CodedInputStream, std::string>::read_fixed64(raw, t);
                if (ok)
                    v = static_cast<int32_t>(val);
            }
        }
    };

    // uint64_t
    template <>
    struct Serialize<google::protobuf::io::CodedOutputStream, Tag<uint64_t>> {
        google::protobuf::io::CodedOutputStream &doc;

        void from(const Tag<uint64_t> &v) const {
            from(v.get_value(), proto::get_tag_info(v));
        }

        void from(uint64_t v, const TagInfo &t) const {
            if (t.key != "" && !(t.omitempty && detail::is_empty_value(v)))
                from(v, proto::get_field_number(t));
        }

        void from(uint64_t v, int field_number) const {
            doc.WriteTag(
                google::protobuf::internal::WireFormatLite::MakeTag(
                    field_number, google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT
                )
            );
            doc.WriteVarint64(v);
        }
    };

    template <>
    struct Deserialize<google::protobuf::io::CodedInputStream, Tag<uint64_t>> {
        std::string raw;

        void into(Tag<uint64_t> &v) const {
            into(v.get_value(), proto::get_tag_info(v));
        }

        void into(uint64_t &v, const TagInfo &t) const {
            auto [val, ok] = Parse<google::protobuf::io::CodedInputStream, std::string>::read_fixed64(raw, t);
            if (ok)
                v = val;
        }
    };

    // int64_t
    template <>
    struct Serialize<google::protobuf::io::CodedOutputStream, Tag<int64_t>> {
        google::protobuf::io::CodedOutputStream &doc;

        void from(const Tag<int64_t> &v) const {
            from(v.get_value(), proto::get_tag_info(v));
        }

        void from(int64_t v, const TagInfo &t) const {
            Serialize<google::protobuf::io::CodedOutputStream, Tag<uint64_t>>{doc}.from(static_cast<uint64_t>(v), t);
        }

        void from(int64_t v, int field_number) const {
            Serialize<google::protobuf::io::CodedOutputStream, Tag<uint64_t>>{doc}.from(static_cast<uint64_t>(v), field_number);
        }
    };

    template <>
    struct Deserialize<google::protobuf::io::CodedInputStream, Tag<int64_t>> {
        std::string raw;

        void into(Tag<int64_t> &v) const {
            into(v.get_value(), proto::get_tag_info(v));
        }

        void into(int64_t &v, const TagInfo &t) const {
            auto [val, ok] = Parse<google::protobuf::io::CodedInputStream, std::string>::read_fixed64(raw, t);
            if (ok)
                v = static_cast<int64_t>(val);
        }
    };

    // enum
    template <typename T>
    struct Serialize<google::protobuf::io::CodedOutputStream, Tag<T>, std::enable_if_t<std::is_enum_v<T>>> {
        google::protobuf::io::CodedOutputStream &doc;

        void from(const Tag<T> &v) const {
            from(v.get_value(), proto::get_tag_info(v));
        }

        void from(T v, const TagInfo &t) const {
            Serialize<google::protobuf::io::CodedOutputStream, Tag<int32_t>>{doc}.from(static_cast<int32_t>(v), t);
        }

        void from(T v, int field_number) const {
            Serialize<google::protobuf::io::CodedOutputStream, Tag<int32_t>>{doc}.from(static_cast<int32_t>(v), field_number);
        }
    };

    template <typename T>
    struct Deserialize<google::protobuf::io::CodedInputStream, Tag<T>, std::enable_if_t<std::is_enum_v<T>>> {
        const std::string &raw;

        void into(Tag<T> &v) const {
            into(v.get_value(), proto::get_tag_info(v));
        }

        void into(T &v, const TagInfo &t) const {
            int32_t i;
            Deserialize<google::protobuf::io::CodedInputStream, Tag<int32_t>>{raw}.into(i, t);
            v = static_cast<T>(i);
        }
    };

    // float
    template <>
    struct Serialize<google::protobuf::io::CodedOutputStream, Tag<float>> {
        google::protobuf::io::CodedOutputStream &doc;

        void from(const Tag<float> &v) const {
            from(v.get_value(), proto::get_tag_info(v));
        }

        void from(float v, const TagInfo &t) const {
            if (t.key != "" && !(t.omitempty && detail::is_empty_value(v)))
                from(v, proto::get_field_number(t));
        }

        void from(float v, int field_number) const {
            doc.WriteTag(
                google::protobuf::internal::WireFormatLite::MakeTag(
                    field_number, google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED32
                )
            );
            uint32_t bits;
            static_assert(sizeof(bits) == sizeof(v));
            std::memcpy(&bits, &v, sizeof(bits));
            doc.WriteLittleEndian32(bits);
        }
    };

    // double
    template <>
    struct Serialize<google::protobuf::io::CodedOutputStream, Tag<double>> {
        google::protobuf::io::CodedOutputStream &doc;

        void from(const Tag<double> &v) const {
            from(v.get_value(), proto::get_tag_info(v));
        }

        void from(double v, const TagInfo &t) const {
            if (t.key != "" && !(t.omitempty && detail::is_empty_value(v)))
                from(v, proto::get_field_number(t));
        }

        void from(double v, int field_number) const {
            doc.WriteTag(
                google::protobuf::internal::WireFormatLite::MakeTag(
                    field_number, google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED32
                )
            );
            uint64_t bits;
            static_assert(sizeof(bits) == sizeof(v));
            std::memcpy(&bits, &v, sizeof(bits));
            doc.WriteLittleEndian64(bits);
        }
    };

    // string
    template <>
    struct Serialize<google::protobuf::io::CodedOutputStream, Tag<std::string>> {
        google::protobuf::io::CodedOutputStream &doc;

        void from(const Tag<std::string> &v) const {
            from(v.get_value(), proto::get_tag_info(v));
        }

        void from(const std::string &v, const TagInfo &t) const {
            if (t.key != "" && !(t.omitempty && detail::is_empty_value(v)))
                from(v, proto::get_field_number(t));
        }

        void from(const std::string &v, int field_number) const {
            doc.WriteTag(
                google::protobuf::internal::WireFormatLite::MakeTag(
                    field_number, google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED
                )
            );
            doc.WriteVarint32(static_cast<uint32_t>(v.size()));
            doc.WriteString(v);
        }
    };

    // bytes
    template <>
    struct Serialize<google::protobuf::io::CodedOutputStream, Tag<std::vector<uint8_t>>> {
        google::protobuf::io::CodedOutputStream &doc;

        void from(const Tag<std::vector<uint8_t>> &v) const {
            from(v.get_value(), proto::get_tag_info(v));
        }

        void from(const std::vector<uint8_t> &v, const TagInfo &t) const {
            if (t.key != "" && !(t.omitempty && detail::is_empty_value(v)))
                from(v, proto::get_field_number(t));
        }

        void from(const std::vector<uint8_t> &v, int field_number) const {
            doc.WriteTag(
                google::protobuf::internal::WireFormatLite::MakeTag(
                    field_number, google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED
                )
            );
            doc.WriteVarint32(static_cast<uint32_t>(v.size()));
            doc.WriteRaw(v.data(), static_cast<int>(v.size()));
        }
    };

    // optional
    template <typename T>
    struct Serialize<google::protobuf::io::CodedOutputStream, Tag<std::optional<T>>> {
        google::protobuf::io::CodedOutputStream &doc;

        void from(const Tag<std::optional<T>> &v) const {
            from(v.get_value(), proto::get_tag_info(v));
        }

        void from(const std::optional<T> &v, const TagInfo &t) {
            if (t.key != "" && !(t.omitempty && detail::is_empty_value(v)))
                from(v, proto::get_field_number(t));
        }

        void from(const std::optional<T> &v, int field_number) {
            if (v.has_value())
                Serialize<google::protobuf::io::CodedOutputStream, Tag<T>>{doc}.from(*v, field_number);
        }
    };

    // repeated
    template <typename T, size_t N>
    struct Serialize<
        google::protobuf::io::CodedOutputStream,
        Tag<std::array<T, N>>,
        std::enable_if_t<!std::is_same_v<T, uint8_t>>> {
        google::protobuf::io::CodedOutputStream &doc;

        void from(const Tag<std::array<T, N>> &v) const {
            from(v.get_value(), proto::get_tag_info(v));
        }

        void from(const std::array<T, N> &v, const TagInfo &t) const {
            if (t.key != "" && !(t.omitempty && detail::is_empty_value(v)))
                from(v, proto::get_field_number(t));
        }

        void from(const std::array<T, N> &arr, int field_number) const {
            for (const auto &v : arr)
                Serialize<google::protobuf::io::CodedOutputStream, Tag<T>>{doc}.from(v, field_number);
        }
    };

    template <typename T>
    struct Serialize<
        google::protobuf::io::CodedOutputStream,
        Tag<std::vector<T>>,
        std::enable_if_t<!std::is_same_v<T, uint8_t>>> {
        google::protobuf::io::CodedOutputStream &doc;

        void from(const Tag<std::vector<T>> &v) const {
            from(v.get_value(), proto::get_tag_info(v));
        }

        void from(const std::vector<T> &v, const TagInfo &t) const {
            if (t.key != "" && !(t.omitempty && detail::is_empty_value(v)))
                from(v, proto::get_field_number(t));
        }

        void from(const std::vector<T> &arr, int field_number) const {
            for (const auto &v : arr)
                Serialize<google::protobuf::io::CodedOutputStream, Tag<T>>{doc}.from(v, field_number);
        }
    };

    // message
    template <typename... Ts>
    struct Serialize<google::protobuf::io::CodedOutputStream, Tag<std::tuple<Ts...>>> {
        google::protobuf::io::CodedOutputStream &doc;

        void from(const Tag<std::tuple<Ts...>> &v) const {
            from(v.get_value(), proto::get_tag_info(v));
        }

        void from(const std::tuple<Ts...> &v, const TagInfo &t) const {
            if (t.key != "" && !(t.omitempty && detail::is_empty_value(v)))
                from(v, proto::get_field_number(t));
        }

        void from(const std::tuple<Ts...> &tpl, int field_number) const {
            std::string buffer = Dump<google::protobuf::io::CodedOutputStream, std::string>{}.from(tpl);
            Serialize<google::protobuf::io::CodedOutputStream, Tag<std::string>>{doc}.from(buffer, field_number);
        }
    };

#ifdef BOOST_PFR_HPP
    template <typename S>
    struct Serialize<google::protobuf::io::CodedOutputStream, Tag<S>, std::enable_if_t<std::is_aggregate_v<S>>> {
        google::protobuf::io::CodedOutputStream &doc;

        void from(const Tag<S> &v) const {
            from(v.get_value(), proto::get_tag_info(v));
        }

        void from(const S &v, const TagInfo &t) const {
            if (t.key != "" && !(t.omitempty && detail::is_empty_value(v)))
                from(v, proto::get_field_number(t));
        }

        void from(const S &v, int field_number) const {
            std::string buffer = Dump<google::protobuf::io::CodedOutputStream, std::string>{}.from(v);
            Serialize<google::protobuf::io::CodedOutputStream, Tag<std::string>>{doc}.from(buffer, field_number);
        }
    };
#endif
} // namespace cppxx::serde

namespace cppxx::proto::google_protobuf {
    template <typename T>
    [[nodiscard]]
    std::string dump(const T &val) {
        return Dump{}.from(val);
    }
} // namespace cppxx::proto::google_protobuf
#endif
