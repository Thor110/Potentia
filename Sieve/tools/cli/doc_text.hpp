// Documents read as their text, for a training corpus (IDEAS §19.120, 12 November 2026: "the
// training routine reading more kinds of file: documents converted to text"). `sieve ai-train
// --record FOLDER --as-text` puts each document's text in the run in its place, so the run carries
// the text itself and trains again the same without this code.
//
// - HTML and XHTML (.html .htm .xhtml): the tags taken out (and what is in script and style), a
//   line ended at each block (a paragraph, a heading, a list item, a row, a line break), entities
//   read, runs of spaces made one.
// - Word (.docx): word/document.xml's runs of text, a line at each paragraph, tabs and breaks kept.
// - OpenDocument text (.odt): content.xml's paragraphs and headings, its spaces (text:s), tabs and
//   line breaks.
// - EPUB (.epub): each chapter in the book's reading order (its spine), read as HTML.
// - RTF (.rtf): the text, its paragraphs and tabs, Windows-1252 and \u characters; fonts, colours,
//   styles, pictures and other destinations left out.
//
// PDF is not read: its text is drawn glyph by glyph in an order of the page's choosing, and fonts
// may hide which letters they are.
#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <string>

namespace sieve::cli {

// The kind of document a file's name says it is ("HTML", "Word", "OpenDocument", "EPUB", "RTF"),
// or nothing for any other file.
std::optional<std::string> document_kind(const std::string& name);

// The text (UTF-8) of a document of `kind`. Throws std::runtime_error where it does not read.
std::string document_text(const std::string& kind, std::span<const uint8_t> bytes);

// HTML (UTF-8, or Windows-1252 where it is not) as its text: what EPUB's chapters are read with.
std::string html_text(std::string_view html);

} // namespace sieve::cli
