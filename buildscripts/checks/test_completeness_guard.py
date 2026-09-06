#!/usr/bin/env python3
"""Run the candidate-bound completeness regression suite."""
import unittest
from test_completion_evidence import EvidenceTests, NarrativeTests

if __name__ == "__main__":
    unittest.main(verbosity=2)
