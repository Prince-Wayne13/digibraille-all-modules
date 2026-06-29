import importlib.util
import tempfile
import unittest
from pathlib import Path

MODULE_PATH = Path(__file__).resolve().parents[1] / "tools" / "download_voicerss_words.py"
spec = importlib.util.spec_from_file_location("download_voicerss_words", MODULE_PATH)
download_voicerss_words = importlib.util.module_from_spec(spec)
spec.loader.exec_module(download_voicerss_words)


class TestApiKeyRotation(unittest.TestCase):
    def test_load_api_keys_reads_file_and_ignores_comments(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            path = Path(tmpdir) / "keys.txt"
            path.write_text("key1\n\n#comment\nkey2\n", encoding="utf-8")
            self.assertEqual(download_voicerss_words.load_api_keys(None, path), ["key1", "key2"])

    def test_load_api_keys_parses_name_equals_key_format(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            path = Path(tmpdir) / "keys.txt"
            path.write_text("sekinna = abc123\n#comment\nsam = def456\n", encoding="utf-8")
            self.assertEqual(download_voicerss_words.load_api_keys(None, path), ["abc123", "def456"])

    def test_should_retry_with_next_key_for_limit_errors(self) -> None:
        self.assertTrue(download_voicerss_words.should_retry_with_next_key(RuntimeError("ERROR: quota exceeded")))
        self.assertTrue(download_voicerss_words.should_retry_with_next_key(RuntimeError("HTTP Error 429")))
        self.assertFalse(download_voicerss_words.should_retry_with_next_key(RuntimeError("bad word")))


if __name__ == "__main__":
    unittest.main()
