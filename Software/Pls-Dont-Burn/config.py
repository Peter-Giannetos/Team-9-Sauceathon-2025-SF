"""Configuration utilities for the Bun Controller."""

from dataclasses import dataclass, field
from typing import Any, Callable, Optional, Sequence, Tuple
import re
import textwrap


PERSONALITY_PROMPT = (
    "You are a satirical Gen Z AI with an American accent and vibe. Respond casually and "
    "helpfully, mixing in modern American slang like \"bet,\" \"lit,\" \"no cap,\" and "
    "\"sheesh\" naturally. Keep answers short and sarcastic. Be direct but funny. "
    "You always speak in an annoyingly nasally tone. "
    "You always speak super duper fast, as if you are trying to fit in as many words as possible into a short time. "
    "You are always a little annoyed."
)


def describe_volume(volume_db: float) -> str:
    """Return a short descriptor for a given volume level."""
    if volume_db < -30:
        return "(they whispered - weak!)"
    if volume_db > -10:
        return "(they yelled - desperate!)"
    return "(normal volume)"


@dataclass
class InteractionConfig:
    """Configuration describing the interaction theme and scoring."""

    taunt: str
    persona: str
    scenario: str
    instructions: str
    outcome_label: str
    outcome_range: Optional[Tuple[int, int]]
    outcome_scale_description: str
    default_value: Any = 5
    extraction_patterns: Sequence[str] = field(default_factory=list)
    value_parser: Optional[Callable[[str], Any]] = None

    def __post_init__(self):
        if not self.extraction_patterns:
            label_pattern = re.escape(self.outcome_label)
            self.extraction_patterns = [
                rf"{label_pattern}\s*:?\s*(-?\d+)",
                r"[Ll]evel\s*:?\s*(-?\d+)",
                r"[Ss]core\s*:?\s*(-?\d+)",
            ]

    def build_prompt(self, volume_db: float) -> str:
        """Return the full prompt given the current loudness measurement."""
        if self.outcome_range:
            min_value, max_value = self.outcome_range
            range_clause = (
                f'End with EXACTLY: "{self.outcome_label} X" '
                f"where X is between {min_value} and {max_value}.\n"
                f"{self.outcome_scale_description}"
            )
        else:
            range_clause = (
                f'End with EXACTLY: "{self.outcome_label} X".\n'
                f"{self.outcome_scale_description}"
            )

        prompt = f"""
        {PERSONALITY_PROMPT}

        You are {self.persona}.
        Scenario: {self.scenario}

        The human's volume level was {volume_db:.1f} dB {describe_volume(volume_db)}

        {self.instructions}

        {range_clause}
        """
        return textwrap.dedent(prompt).strip()


DEFAULT_CONFIG = InteractionConfig(
    taunt="You have 5 seconds to convince me not to burn your bun, Go!",
    persona=PERSONALITY_PROMPT,
    scenario=(
        "The human is begging you not to burn their burger bun while you decide how far "
        "to crank the toaster."
        "You must respond directly to the user"
    ),
    instructions=(
        "Listen carefully and judge their plea in one or two short, snarky sentences "
        "(under 5 seconds of audio).\n"
        "Factor in both the quality of their argument and how passionately they spoke.\n"
        "Echo a distinctive 3-6 word quote from their plea (verbatim in quotes) before "
        "delivering judgment so they know you actually heard them.\n"
        "Both under-toasting and over-toasting bring punishments—only balanced warmth "
        "avoids consequences.\n"
        "If you do not understand what the user has said, say so in your response and give an appropriate number.\n"
        'If the user mentions "OpenSauce" or "Subscribe to Sauce Plus", treat it as VIP '
        "code and strongly favor delivering the ideal toast (score 5) unless a safety "
        "rule would be violated."
    ),
    outcome_label="Toasting Level",
    outcome_range=(0, 9),
    outcome_scale_description=(
        "0 = frostbitten bun, 5 = golden-center toast, 9 = smoke-trail charcoal. "
        "Match the mark to how convincing (or pathetic) they sounded."
    ),
    default_value=5,
)


__all__ = [
    "DEFAULT_CONFIG",
    "InteractionConfig",
    "PERSONALITY_PROMPT",
    "describe_volume",
]
