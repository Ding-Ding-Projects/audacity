import importlib.util
import pathlib
import unittest


SOURCE = pathlib.Path(__file__).parents[1] / "acquire_official_catalog.py"
SPEC = importlib.util.spec_from_file_location("catalog", SOURCE)
catalog = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(catalog)


class OfficialCatalogParserTests(unittest.TestCase):
    def test_index_extracts_unique_model_links(self):
        parser = catalog.Links()
        parser.feed('<a href="/library/llama3.2">one</a><a href="/library/llama3.2">two</a>')
        self.assertEqual(parser.models, {"llama3.2"})

    def test_rel_next_is_not_terminal(self):
        parser = catalog.Links()
        parser.feed('<a rel="next" href="/library?page=2">next</a>')
        self.assertEqual(parser.next_url, "/library?page=2")

    def test_htmx_next_is_not_terminal(self):
        parser = catalog.Links()
        parser.feed('<button hx-get="/library?page=2">more</button>')
        self.assertEqual(parser.next_url, "/library?page=2")

    def test_data_next_is_not_terminal(self):
        parser = catalog.Links()
        parser.feed('<a data-next="/library?page=2">more</a>')
        self.assertEqual(parser.next_url, "/library?page=2")

    def test_rejects_public_and_non_library_urls(self):
        for url in ("http://ollama.com/library", "https://example.test/library", "https://ollama.com/blog"):
            with self.assertRaises(ValueError):
                catalog.canonical(url)

    def test_empty_tag_receipt_fails(self):
        old_fetch = catalog.fetch
        try:
            catalog.fetch = lambda url: (url, b'<html><body>no tags</body></html>')
            with self.assertRaises(ValueError):
                catalog.detail_receipt("llama3.2")
        finally:
            catalog.fetch = old_fetch

    def test_lazy_tag_receipt_fails(self):
        old_fetch = catalog.fetch
        try:
            catalog.fetch = lambda url: (url, b'<a href="/library/llama3.2:1b">tag</a><button hx-get="/library/llama3.2/tags?page=2">more</button>')
            with self.assertRaises(ValueError):
                catalog.detail_receipt("llama3.2")
        finally:
            catalog.fetch = old_fetch

    def test_tag_receipt_deduplicates_ids(self):
        old_fetch = catalog.fetch
        try:
            catalog.fetch = lambda url: (url, b'<a href="/library/llama3.2:1b">tag</a><a href="/library/llama3.2:1b">tag</a>')
            receipt = catalog.detail_receipt("llama3.2")
            self.assertEqual(receipt["tags"], ["llama3.2:1b"])
        finally:
            catalog.fetch = old_fetch


if __name__ == "__main__":
    unittest.main()
