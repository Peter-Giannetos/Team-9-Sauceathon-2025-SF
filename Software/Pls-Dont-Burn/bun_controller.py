"""CLI entry point for the AI-powered burger bun toast controller."""

from __future__ import annotations

import argparse
import logging
import os
import sys

from controller import BunController

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    datefmt="%H:%M:%S",
)
logger = logging.getLogger(__name__)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Parse CLI arguments."""
    parser = argparse.ArgumentParser(description="Run the bun controller interaction loop.")
    parser.add_argument("--debug", action="store_true", help="Enable verbose debug logging.")
    return parser.parse_args(argv)


def enable_debug_logging(source: str):
    """Switch logging to DEBUG for all modules."""
    root_logger = logging.getLogger()
    root_logger.setLevel(logging.DEBUG)
    logger.debug("Debug logging enabled via %s", source)


def print_banner():
    """Render the intro banner and configuration reminder."""
    print("Welcome to the AI Burger Bun Toast Controller!")
    print("   Powered by Gemini Live API")
    print("\nMake sure you have set up your .env file with:")
    print("  - GOOGLE_API_KEY")
    print()


def main(argv: list[str] | None = None) -> int:
    """Program entry point."""
    args = parse_args(argv)

    if os.getenv("DEBUG", "").lower() in ("1", "true", "yes"):
        enable_debug_logging("DEBUG environment variable")
    if args.debug:
        enable_debug_logging("--debug flag")

    print_banner()

    if not os.getenv("GOOGLE_API_KEY"):
        print("Error: GOOGLE_API_KEY not found in .env file")
        print("   Get your API key at: https://ai.google.dev/")
        return 1

    try:
        controller = BunController()
        outcome_value = controller.run_cycle()
        print(f"\nFinal {controller.config.outcome_label.lower()} to send to hardware: {outcome_value}")
    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
        return 130
    except Exception as exc:  # pragma: no cover - surfaced to operator
        logger.error("Error: %s", exc, exc_info=True)
        print(f"\nError: {exc}")
        print("   Run with --debug flag for more details")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
