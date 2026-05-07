#!/usr/bin/env python3
"""Matplotlib 3D visualizer for building ray tracing acceleration data."""

from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass
from typing import Callable, Optional, Sequence


MODES = ("grid", "bvh", "tlas-blas")


@dataclass(frozen=True)
class Box3:
    x0: float
    y0: float
    z0: float
    x1: float
    y1: float
    z1: float

    @property
    def cx(self) -> float:
        return (self.x0 + self.x1) * 0.5

    @property
    def cy(self) -> float:
        return (self.y0 + self.y1) * 0.5

    @property
    def cz(self) -> float:
        return (self.z0 + self.z1) * 0.5

    @property
    def width(self) -> float:
        return self.x1 - self.x0

    @property
    def depth(self) -> float:
        return self.y1 - self.y0

    @property
    def height(self) -> float:
        return self.z1 - self.z0

    def intersects(self, other: "Box3") -> bool:
        return not (
            self.x1 < other.x0
            or self.x0 > other.x1
            or self.y1 < other.y0
            or self.y0 > other.y1
            or self.z1 < other.z0
            or self.z0 > other.z1
        )

    def translated(self, dx: float, dy: float, dz: float) -> "Box3":
        return Box3(self.x0 + dx, self.y0 + dy, self.z0 + dz, self.x1 + dx, self.y1 + dy, self.z1 + dz)


@dataclass(frozen=True)
class SceneObject:
    name: str
    box: Box3
    color: str


@dataclass(frozen=True)
class BvhNode:
    box: Box3
    objects: tuple[SceneObject, ...]
    left: Optional["BvhNode"] = None
    right: Optional["BvhNode"] = None
    depth: int = 0
    label: str = "root"
    split_axis: str = "-"

    @property
    def is_leaf(self) -> bool:
        return self.left is None and self.right is None


@dataclass(frozen=True)
class BuildStep:
    title: str
    detail: str
    draw: Callable[["BuildVisualizer"], None]
    visited_nodes: int = 0
    primitive_refs: int = 0
    result: str = ""


WORLD = Box3(0, 0, 0, 90, 60, 45)
OBJECTS = (
    SceneObject("A", Box3(8, 7, 0, 20, 19, 14), "#e76f51"),
    SceneObject("B", Box3(33, 8, 0, 46, 22, 22), "#2a9d8f"),
    SceneObject("C", Box3(60, 9, 0, 74, 24, 16), "#e9c46a"),
    SceneObject("D", Box3(18, 38, 0, 36, 53, 18), "#f4a261"),
    SceneObject("E", Box3(58, 34, 0, 78, 53, 26), "#8ab17d"),
)
LOCAL_PARTS = (
    (
        SceneObject("M1-a", Box3(0, 0, 0, 10, 10, 12), "#e76f51"),
        SceneObject("M1-b", Box3(13, 5, 0, 25, 17, 16), "#f4a261"),
        SceneObject("M1-c", Box3(5, 21, 0, 20, 33, 11), "#e9c46a"),
    ),
    (
        SceneObject("M2-a", Box3(0, 3, 0, 13, 17, 18), "#2a9d8f"),
        SceneObject("M2-b", Box3(18, 0, 0, 31, 13, 14), "#577590"),
        SceneObject("M2-c", Box3(10, 21, 0, 29, 34, 24), "#b56576"),
    ),
)
INSTANCE_OFFSETS = ((10, 8, 0), (51, 24, 0))


def union_box(boxes: Sequence[Box3]) -> Box3:
    return Box3(
        min(box.x0 for box in boxes),
        min(box.y0 for box in boxes),
        min(box.z0 for box in boxes),
        max(box.x1 for box in boxes),
        max(box.y1 for box in boxes),
        max(box.z1 for box in boxes),
    )


def scene_bounds(objects: tuple[SceneObject, ...]) -> Box3:
    return union_box(tuple(obj.box for obj in objects))


def build_bvh(objects: tuple[SceneObject, ...], depth: int = 0, label: str = "root") -> BvhNode:
    bounds = scene_bounds(objects)
    if len(objects) <= 2:
        return BvhNode(bounds, objects, depth=depth, label=label)
    axis = max((("x", bounds.width), ("y", bounds.depth), ("z", bounds.height)), key=lambda item: item[1])[0]
    key = {"x": lambda obj: obj.box.cx, "y": lambda obj: obj.box.cy, "z": lambda obj: obj.box.cz}[axis]
    sorted_objects = tuple(sorted(objects, key=key))
    mid = len(sorted_objects) // 2
    return BvhNode(
        bounds,
        objects,
        build_bvh(sorted_objects[:mid], depth + 1, f"{label}.L"),
        build_bvh(sorted_objects[mid:], depth + 1, f"{label}.R"),
        depth,
        label,
        axis,
    )


def collect_nodes(node: BvhNode) -> list[BvhNode]:
    nodes = [node]
    if node.left:
        nodes.extend(collect_nodes(node.left))
    if node.right:
        nodes.extend(collect_nodes(node.right))
    return nodes


def grid_cells(nx: int, ny: int, nz: int) -> list[Box3]:
    cells: list[Box3] = []
    sx = WORLD.width / nx
    sy = WORLD.depth / ny
    sz = WORLD.height / nz
    for iz in range(nz):
        for iy in range(ny):
            for ix in range(nx):
                cells.append(Box3(ix * sx, iy * sy, iz * sz, (ix + 1) * sx, (iy + 1) * sy, (iz + 1) * sz))
    return cells


def require_matplotlib():
    try:
        import matplotlib.pyplot as plt
        from matplotlib.widgets import Button, RadioButtons, Slider
        from mpl_toolkits.mplot3d.art3d import Poly3DCollection
    except ModuleNotFoundError:
        print("Matplotlib is required. Install it with: python3 -m pip install -r tools/requirements.txt", file=sys.stderr)
        raise SystemExit(1)
    return plt, Button, RadioButtons, Slider, Poly3DCollection


class BuildVisualizer:
    def __init__(self, default_mode: str) -> None:
        self.plt, self.Button, self.RadioButtons, self.Slider, self.Poly3DCollection = require_matplotlib()
        self.mode = default_mode
        self.step_index = 0
        self.steps: list[BuildStep] = []
        self.autoplay = False
        self.timer = None

        self.fig = self.plt.figure(figsize=(12, 8))
        self.ax = self.fig.add_axes((0.06, 0.18, 0.72, 0.74), projection="3d")
        self.fig.canvas.manager.set_window_title("Ray Tracing Build Visualizer - Matplotlib 3D")

        self.mode_ax = self.fig.add_axes((0.82, 0.58, 0.14, 0.24))
        self.mode_buttons = self.RadioButtons(self.mode_ax, MODES, active=MODES.index(default_mode))
        self.mode_buttons.on_clicked(self.set_mode)
        self.reset_button = self.Button(self.fig.add_axes((0.82, 0.47, 0.14, 0.05)), "Reset")
        self.step_button = self.Button(self.fig.add_axes((0.82, 0.40, 0.14, 0.05)), "Step")
        self.play_button = self.Button(self.fig.add_axes((0.82, 0.33, 0.14, 0.05)), "Autoplay")
        self.speed_slider = self.Slider(self.fig.add_axes((0.82, 0.24, 0.14, 0.04)), "Delay", 150, 1200, valinit=650, valfmt="%0.0f ms")

        self.reset_button.on_clicked(lambda _event: self.reset())
        self.step_button.on_clicked(lambda _event: self.step())
        self.play_button.on_clicked(lambda _event: self.toggle_autoplay())
        self.rebuild()

    def run(self) -> None:
        self.plt.show()

    def set_mode(self, mode: str) -> None:
        self.mode = mode
        self.rebuild()

    def rebuild(self) -> None:
        self.autoplay = False
        self.step_index = 0
        self.steps = {
            "grid": self.make_grid_steps,
            "bvh": self.make_bvh_steps,
            "tlas-blas": self.make_tlas_blas_steps,
        }[self.mode]()
        self.render()

    def reset(self) -> None:
        self.step_index = 0
        self.render()

    def step(self) -> None:
        self.step_index = min(self.step_index + 1, len(self.steps) - 1)
        self.render()

    def toggle_autoplay(self) -> None:
        self.autoplay = not self.autoplay
        if self.autoplay:
            self.schedule_timer()

    def schedule_timer(self) -> None:
        if self.timer:
            self.timer.stop()
        self.timer = self.fig.canvas.new_timer(interval=int(self.speed_slider.val))
        self.timer.add_callback(self.autoplay_tick)
        self.timer.start()

    def autoplay_tick(self) -> None:
        if not self.autoplay:
            return
        if self.step_index >= len(self.steps) - 1:
            self.autoplay = False
            return
        self.step()
        self.schedule_timer()

    def render(self) -> None:
        elev, azim = self.ax.elev, self.ax.azim
        self.ax.clear()
        self.setup_axes(elev, azim)
        step = self.steps[self.step_index]
        step.draw(self)
        self.draw_legend()
        self.fig.suptitle(step.title, fontsize=15, fontweight="bold")
        self.ax.set_title(step.detail, fontsize=10, loc="left", pad=12)
        footer = f"Step {self.step_index + 1}/{len(self.steps)} | Nodes/cells built: {step.visited_nodes} | Primitive refs: {step.primitive_refs}"
        if step.result:
            footer += f" | {step.result}"
        self.fig.text(0.06, 0.08, footer, fontsize=10)
        self.fig.canvas.draw_idle()

    def setup_axes(self, elev: float, azim: float) -> None:
        self.ax.set_xlim(WORLD.x0, WORLD.x1)
        self.ax.set_ylim(WORLD.y0, WORLD.y1)
        self.ax.set_zlim(WORLD.z0, WORLD.z1)
        self.ax.set_xlabel("x")
        self.ax.set_ylabel("y")
        self.ax.set_zlabel("z")
        self.ax.view_init(elev=elev if elev is not None else 24, azim=azim if azim is not None else -55)
        self.ax.set_box_aspect((WORLD.width, WORLD.depth, WORLD.height))

    def box_vertices(self, box: Box3) -> list[tuple[float, float, float]]:
        return [
            (box.x0, box.y0, box.z0),
            (box.x1, box.y0, box.z0),
            (box.x1, box.y1, box.z0),
            (box.x0, box.y1, box.z0),
            (box.x0, box.y0, box.z1),
            (box.x1, box.y0, box.z1),
            (box.x1, box.y1, box.z1),
            (box.x0, box.y1, box.z1),
        ]

    def draw_box(self, box: Box3, color: str, alpha: float = 0.08, linewidth: float = 1.5, label: str = "") -> None:
        pts = self.box_vertices(box)
        faces = [[pts[i] for i in face] for face in ((0, 1, 2, 3), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7))]
        collection = self.Poly3DCollection(faces, facecolors=color, edgecolors=color, linewidths=linewidth, alpha=alpha)
        self.ax.add_collection3d(collection)
        if label:
            self.ax.text(box.cx, box.cy, box.z1 + 1.2, label, color=color, fontsize=9, ha="center")

    def draw_world(self) -> None:
        self.draw_box(WORLD, "#52616b", alpha=0.015, linewidth=1.0, label="scene")

    def draw_objects(self, objects: tuple[SceneObject, ...] = OBJECTS) -> None:
        for obj in objects:
            self.draw_box(obj.box, obj.color, alpha=0.38, linewidth=1.4, label=obj.name)

    def draw_grid(self, nx: int, ny: int, nz: int, color: str = "#94a3b8") -> list[Box3]:
        cells = grid_cells(nx, ny, nz)
        for cell in cells:
            self.draw_box(cell, color, alpha=0.015, linewidth=0.6)
        return cells

    def draw_legend(self) -> None:
        handles = [
            self.plt.Line2D([0], [0], color="#2a9d8f", lw=5, label="object"),
            self.plt.Line2D([0], [0], color="#2563eb", lw=3, label="bounds"),
            self.plt.Line2D([0], [0], color="#d97706", lw=3, label="current"),
            self.plt.Line2D([0], [0], color="#0f766e", lw=3, label="complete"),
        ]
        self.ax.legend(handles=handles, loc="upper left", bbox_to_anchor=(0.0, 1.02), fontsize=8)

    def make_grid_steps(self) -> list[BuildStep]:
        nx, ny, nz = 3, 3, 2
        cells = grid_cells(nx, ny, nz)
        assignments = [(obj, [i for i, cell in enumerate(cells) if cell.intersects(obj.box)]) for obj in OBJECTS]
        steps: list[BuildStep] = []

        def draw_scene(app: BuildVisualizer) -> None:
            app.draw_world()
            app.draw_objects()

        steps.append(BuildStep("3D uniform grid build", "Start with primitives in 3D and one scene bounding box.", draw_scene, 1, len(OBJECTS)))

        def draw_grid_only(app: BuildVisualizer) -> None:
            app.draw_world()
            app.draw_grid(nx, ny, nz)
            app.draw_objects()

        steps.append(BuildStep("Subdivide x, y, and z", "The grid creates fixed cells across the whole volume.", draw_grid_only, nx * ny * nz, 0))

        for index, (obj, _indices) in enumerate(assignments):
            assigned = assignments[: index + 1]

            def draw_assignment(app: BuildVisualizer, assigned_so_far=assigned, current=obj) -> None:
                app.draw_world()
                app.draw_grid(nx, ny, nz)
                for _obj, cell_indices in assigned_so_far:
                    for cell_index in cell_indices:
                        app.draw_box(cells[cell_index], "#d97706", alpha=0.16, linewidth=1.2)
                app.draw_objects()
                app.draw_box(current.box, "#dc2626", alpha=0.42, linewidth=2.4, label=current.name)

            steps.append(BuildStep(f"Insert object {obj.name}", "Store primitive references in every overlapped 3D cell.", draw_assignment, nx * ny * nz, sum(len(v) for _, v in assigned)))

        steps.append(BuildStep("Grid complete", "Traversal can skip empty volumes and test only referenced primitives.", draw_grid_only, nx * ny * nz, sum(len(v) for _, v in assignments), "ready for 3D DDA traversal"))
        return steps

    def make_bvh_steps(self) -> list[BuildStep]:
        nodes = collect_nodes(build_bvh(OBJECTS))
        colors = ("#0f766e", "#2563eb", "#7c3aed", "#be123c")
        steps: list[BuildStep] = []
        for count in range(1, len(nodes) + 1):
            visible = nodes[:count]
            current = visible[-1]

            def draw_bvh(app: BuildVisualizer, visible_nodes=visible, current_node=current) -> None:
                app.draw_world()
                app.draw_objects()
                for node in visible_nodes:
                    color = "#d97706" if node is current_node else colors[min(node.depth, len(colors) - 1)]
                    app.draw_box(node.box, color, alpha=0.08, linewidth=2.4 if node is current_node else 1.4, label=node.label)

            action = "leaf" if current.is_leaf else f"split along {current.split_axis}"
            steps.append(BuildStep(f"BVH node {current.label}", f"Tight 3D AABB for {len(current.objects)} primitive(s), then {action}.", draw_bvh, count, sum(len(node.objects) for node in visible)))
        return steps

    def make_tlas_blas_steps(self) -> list[BuildStep]:
        instances: list[tuple[str, tuple[SceneObject, ...], Box3]] = []
        for index, (parts, offset) in enumerate(zip(LOCAL_PARTS, INSTANCE_OFFSETS), start=1):
            world_parts = tuple(SceneObject(part.name, part.box.translated(*offset), part.color) for part in parts)
            instances.append((f"Instance {index}", world_parts, scene_bounds(world_parts)))
        tlas_bounds = union_box(tuple(bounds for _name, _parts, bounds in instances))
        built: list[tuple[str, tuple[SceneObject, ...], Box3]] = []
        steps: list[BuildStep] = []

        def draw_instances(app: BuildVisualizer, show_tlas: bool = False) -> None:
            app.draw_world()
            for name, parts, bounds in instances:
                app.draw_objects(parts)
                app.draw_box(bounds, "#2563eb", alpha=0.08, linewidth=1.8, label=name)
            if show_tlas:
                app.draw_box(tlas_bounds, "#be123c", alpha=0.06, linewidth=2.4, label="TLAS root")

        steps.append(BuildStep("TLAS/BLAS build", "Build local BLAS bounds first, then a top-level structure over instances.", lambda app: draw_instances(app), 0, 0))

        for name, parts, bounds in instances:
            built.append((name, parts, bounds))

            def draw_blas(app: BuildVisualizer, current=name) -> None:
                app.draw_world()
                for built_name, built_parts, built_bounds in built:
                    app.draw_objects(built_parts)
                    app.draw_box(built_bounds, "#0f766e" if built_name == current else "#64748b", alpha=0.10, linewidth=2.4 if built_name == current else 1.2, label=built_name)

            steps.append(BuildStep(f"Build BLAS for {name}", "A BLAS groups mesh-local primitive bounds before instancing.", draw_blas, len(built), sum(len(parts) for _name, parts, _bounds in built)))

        def draw_tlas_leaves(app: BuildVisualizer) -> None:
            draw_instances(app)
            for name, _parts, bounds in instances:
                app.draw_box(bounds, "#7c3aed", alpha=0.10, linewidth=2.2, label=f"TLAS leaf {name[-1]}")

        steps.append(BuildStep("Create TLAS leaves", "The top level stores instance boxes instead of every primitive box.", draw_tlas_leaves, len(instances), len(instances)))
        steps.append(BuildStep("TLAS complete", "Rays test the TLAS first, then descend only into hit BLAS structures.", lambda app: draw_instances(app, True), len(instances) + 1, len(instances), "ready for instanced traversal"))
        return steps


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Visualize construction of grid, BVH, and TLAS/BLAS acceleration structures in Matplotlib 3D.")
    parser.add_argument("--mode", choices=MODES, default="grid", help="Initial visualization mode.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    BuildVisualizer(args.mode).run()


if __name__ == "__main__":
    main()
