"""
Tests that built-in shape types are emitted correctly.
"""

import os.path
import subprocess
import sys
from pathlib import Path

import pytest

# Import helper function to compare graphs from tests/regressions_tests
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from regression_test_helpers import (  # pylint: disable=import-error,wrong-import-position
    compare_graphs,
)

shapes = [
    "box",
    "polygon",
    "ellipse",
    "oval",
    "circle",
    "point",
    "egg",
    "triangle",
    "none",
    "plaintext",
    "plain",
    "diamond",
    "trapezium",
    "parallelogram",
    "house",
    "pentagon",
    "hexagon",
    "septagon",
    "octagon",
    "note",
    "tab",
    "folder",
    "box3d",
    "component",
    "cylinder",
    "rect",
    "rectangle",
    "square",
    "star",
    "doublecircle",
    "doubleoctagon",
    "tripleoctagon",
    "invtriangle",
    "invtrapezium",
    "invhouse",
    "underline",
    "Mdiamond",
    "Msquare",
    "Mcircle",
    # biological circuit shapes
    # gene expression symbols
    "promoter",
    "cds",
    "terminator",
    "utr",
    "insulator",
    "ribosite",
    "rnastab",
    "proteasesite",
    "proteinstab",
    # dna construction symbols
    "primersite",
    "restrictionsite",
    "fivepoverhang",
    "threepoverhang",
    "noverhang",
    "assembly",
    "signature",
    "rpromoter",
    "larrow",
    "rarrow",
    "lpromoter",
]

output_types = ["gv", "svg", "xdot"]


def generate_shape_graph(shape, output_type):
    """
    Produce a graph of the given shape and output format.
    """
    if not Path("output").exists():
        Path("output").mkdir(parents=True)

    output_file = Path("output") / f"{shape}.{output_type}"
    input_graph = f'graph G {{ a [label="" shape={shape}] }}'
    try:
        subprocess.run(
            ["dot", f"-T{output_type}", "-o", output_file],
            input=input_graph.encode("utf_8"),
            check=True,
        )
    except subprocess.CalledProcessError:
        print(f"An error occurred while generating: {output_file}")
        sys.exit(1)

    if output_type == "svg":
        # Remove the number in "Generated by graphviz version <number>"
        # to able to compare the output to the reference. This version
        # number is different for every Graphviz compilation.
        with open(output_file, "rt", encoding="utf-8") as file:
            lines = file.readlines()

        with open(output_file, "wt", encoding="utf-8") as file:
            for line in lines:
                if "<!-- Generated by graphviz version " in line:
                    file.write("<!-- Generated by graphviz version\n")
                else:
                    file.write(line)


@pytest.mark.parametrize(
    "shape,output_type",
    [(shape, output_type) for shape in shapes for output_type in output_types],
)
def test_shape(shape, output_type):
    """
    Check a shape corresponds to its reference.
    """
    os.chdir(Path(__file__).resolve().parent)
    generate_shape_graph(shape, output_type)
    assert compare_graphs(shape, output_type)
