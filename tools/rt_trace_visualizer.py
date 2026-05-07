#!/usr/bin/env python3
"""Matplotlib 3D visualizer for ray traversal through acceleration data."""

from __future__ import annotations

import argparse
import math
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
class TraceStep:
    title: str
    detail: str
    draw: Callable[["TraceVisualizer"], None]
    visited: int = 0
    primitive_tests: int = 0
    result: str = "searching"


WORLD = Box3(0, 0, 0, 90, 60, 45)
RAY_ORIGIN = (-8.0, 18.0, 12.0)
RAY_DIRECTION = (1.0, 0.30, 0.02)
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


def ray_box_t(box: Box3, origin: tuple[float, float, float] = RAY_ORIGIN, direction: tuple[float, float, float] = RAY_DIRECTION) -> tuple[bool, float, float]:
    tmin = -math.inf
    tmax = math.inf
    for start, delta, low, high in (
        (origin[0], direction[0], box.x0, box.x1),
        (origin[1], direction[1], box.y0, box.y1),
        (origin[2], direction[2], box.z0, box.z1),
    ):
        if abs(delta) < 1e-8:
            if start < low or start > high:
                return False, math.inf, math.inf
            continue
        inv = 1.0 / delta
        t0 = (low - start) * inv
        t1 = (high - start) * inv
        if t0 > t1:
            t0, t1 = t1, t0
        tmin = max(tmin, t0)
        tmax = min(tmax, t1)
        if tmax < tmin:
            return False, math.inf, math.inf
    if tmax < 0:
        return False, math.inf, math.inf
    return True, max(tmin, 0.0), tmax


def ray_point(t: float, origin: tuple[float, float, float] = RAY_ORIGIN, direction: tuple[float, float, float] = RAY_DIRECTION) -> tuple[float, float, float]:
    return origin[0] + direction[0] * t, origin[1] + direction[1] * t, origin[2] + direction[2] * t


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


class TraceVisualizer:
    def __init__(self, default_mode: str) -> None:
        self.plt, self.Button, self.RadioButtons, self.Slider, self.Poly3DCollection = require_matplotlib()
        self.mode = default_mode
        self.step_index = 0
        self.steps: list[TraceStep] = []
        self.autoplay = False
        self.timer = None

        self.fig = self.plt.figure(figsize=(12, 8))
        self.ax = self.fig.add_axes((0.06, 0.18, 0.72, 0.74), projection="3d")
        self.fig.canvas.manager.set_window_title("Ray Tracing Traversal Visualizer - Matplotlib 3D")

        self.mode_buttons = self.RadioButtons(self.fig.add_axes((0.82, 0.58, 0.14, 0.24)), MODES, active=MODES.index(default_mode))
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
        self.fig.text(0.06, 0.08, f"Step {self.step_index + 1}/{len(self.steps)} | Visited: {step.visited} | Primitive tests: {step.primitive_tests} | Result: {step.result}", fontsize=10)
        self.fig.canvas.draw_idle()

    def setup_axes(self, elev: float, azim: float) -> None:
        self.ax.set_xlim(WORLD.x0 - 8, WORLD.x1)
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

    def draw_grid(self, nx: int, ny: int, nz: int) -> list[Box3]:
        cells = grid_cells(nx, ny, nz)
        for cell in cells:
            self.draw_box(cell, "#94a3b8", alpha=0.015, linewidth=0.6)
        return cells

    def draw_ray(self, until_t: float = 105.0, color: str = "#111827", linewidth: float = 2.5) -> None:
        start = ray_point(0.0)
        end = ray_point(until_t)
        self.ax.plot([start[0], end[0]], [start[1], end[1]], [start[2], end[2]], color=color, linewidth=linewidth)
        self.ax.scatter([start[0]], [start[1]], [start[2]], color=color, s=25)

    def draw_hit_marker(self, t: float) -> None:
        point = ray_point(t)
        self.ax.scatter([point[0]], [point[1]], [point[2]], color="#dc2626", s=90, marker="x", linewidths=3)
        self.ax.text(point[0], point[1], point[2] + 3, "closest hit", color="#dc2626", fontsize=10)

    def base_scene(self, objects: tuple[SceneObject, ...] = OBJECTS) -> None:
        self.draw_world()
        self.draw_objects(objects)
        self.draw_ray()

    def draw_legend(self) -> None:
        handles = [
            self.plt.Line2D([0], [0], color="#2a9d8f", lw=5, label="object"),
            self.plt.Line2D([0], [0], color="#2563eb", lw=3, label="bounds"),
            self.plt.Line2D([0], [0], color="#0284c7", lw=3, label="visited"),
            self.plt.Line2D([0], [0], color="#d97706", lw=3, label="tested"),
            self.plt.Line2D([0], [0], color="#dc2626", lw=3, label="hit"),
        ]
        self.ax.legend(handles=handles, loc="upper left", bbox_to_anchor=(0.0, 1.02), fontsize=8)

    def closest_object_hit(self, objects: tuple[SceneObject, ...]) -> tuple[Optional[SceneObject], float]:
        hits = [(obj, ray_box_t(obj.box)[1]) for obj in objects if ray_box_t(obj.box)[0]]
        if not hits:
            return None, math.inf
        return min(hits, key=lambda item: item[1])

    def make_grid_steps(self) -> list[TraceStep]:
        nx, ny, nz = 3, 3, 2
        cells = grid_cells(nx, ny, nz)
        assignments = {index: [obj for obj in OBJECTS if cell.intersects(obj.box)] for index, cell in enumerate(cells)}
        visited = [index for index, _t in sorted(((index, ray_box_t(cell)[1]) for index, cell in enumerate(cells) if ray_box_t(cell)[0]), key=lambda item: item[1])]
        hit_obj, hit_t = self.closest_object_hit(OBJECTS)
        steps: list[TraceStep] = []

        def draw_start(app: TraceVisualizer) -> None:
            app.draw_world()
            app.draw_grid(nx, ny, nz)
            app.draw_objects()
            app.draw_ray()

        steps.append(TraceStep("3D grid traversal", "Rotate the view while stepping through the cells crossed by the ray.", draw_start, 0, 0))
        total_tests = 0
        for step_number, cell_index in enumerate(visited, start=1):
            tests = assignments[cell_index]
            total_tests += len(tests)
            seen = visited[:step_number]

            def draw_cell(app: TraceVisualizer, seen_cells=seen, current_cell=cell_index, current_tests=tests) -> None:
                app.draw_world()
                app.draw_grid(nx, ny, nz)
                for seen_index in seen_cells:
                    app.draw_box(cells[seen_index], "#d97706" if seen_index == current_cell else "#0284c7", alpha=0.16, linewidth=2.0 if seen_index == current_cell else 1.2)
                app.draw_objects()
                for obj in current_tests:
                    app.draw_box(obj.box, "#d97706", alpha=0.46, linewidth=2.4, label=obj.name)
                app.draw_ray()

            result = f"candidate {hit_obj.name}" if hit_obj and hit_obj in tests else "searching"
            steps.append(TraceStep(f"Visit 3D grid cell {cell_index}", "3D DDA advances to the next x, y, or z cell boundary.", draw_cell, step_number, total_tests, result))
            if hit_obj and hit_obj in tests:
                break

        def draw_finish(app: TraceVisualizer) -> None:
            draw_start(app)
            for seen_index in visited:
                app.draw_box(cells[seen_index], "#0284c7", alpha=0.10, linewidth=1.4)
            if hit_obj:
                app.draw_box(hit_obj.box, "#dc2626", alpha=0.48, linewidth=2.8, label=hit_obj.name)
                app.draw_hit_marker(hit_t)

        steps.append(TraceStep("Grid traversal complete", "The grid skipped empty 3D space and unrelated primitive references.", draw_finish, len(visited), total_tests, f"closest hit {hit_obj.name if hit_obj else 'none'}"))
        return steps

    def make_bvh_steps(self) -> list[TraceStep]:
        root = build_bvh(OBJECTS)
        events: list[tuple[str, Optional[BvhNode], Optional[SceneObject], bool, float]] = []

        def traverse(node: BvhNode) -> None:
            hit, t0, _t1 = ray_box_t(node.box)
            events.append(("node", node, None, hit, t0))
            if not hit:
                return
            if node.is_leaf:
                for obj in node.objects:
                    obj_hit, obj_t, _obj_exit = ray_box_t(obj.box)
                    events.append(("primitive", node, obj, obj_hit, obj_t))
                return
            children = [child for child in (node.left, node.right) if child is not None]
            children.sort(key=lambda child: ray_box_t(child.box)[1])
            for child in children:
                traverse(child)

        traverse(root)
        hit_events = [event for event in events if event[0] == "primitive" and event[3]]
        hit_obj = min(hit_events, key=lambda event: event[4])[2] if hit_events else None
        hit_t = min(hit_events, key=lambda event: event[4])[4] if hit_events else math.inf
        steps: list[TraceStep] = []
        primitive_tests = 0

        def draw_visible(app: TraceVisualizer, visible_events: list[tuple[str, Optional[BvhNode], Optional[SceneObject], bool, float]]) -> None:
            app.base_scene()
            for kind, node, obj, hit, _t in visible_events:
                if kind == "node" and node is not None:
                    app.draw_box(node.box, "#0f766e" if hit else "#94a3b8", alpha=0.10 if hit else 0.04, linewidth=2.0, label=f"{node.label} {'hit' if hit else 'miss'}")
                if kind == "primitive" and obj is not None:
                    app.draw_box(obj.box, "#dc2626" if hit else "#64748b", alpha=0.48 if hit else 0.05, linewidth=2.8 if hit else 1.4, label=obj.name)

        for index, event in enumerate(events, start=1):
            kind, node, obj, hit, _t = event
            primitive_tests += 1 if kind == "primitive" else 0
            visible = events[:index]

            def draw_step(app: TraceVisualizer, visible_events=visible) -> None:
                draw_visible(app, visible_events)

            if kind == "node" and node is not None:
                title = f"Test BVH node {node.label}"
                detail = "A missed 3D AABB prunes every child below it." if not hit else "A hit 3D AABB keeps this subtree in the traversal."
                result = "searching"
            else:
                assert obj is not None
                title = f"Test primitive {obj.name}"
                detail = "Primitive tests happen only inside hit BVH leaves."
                result = f"candidate {obj.name}" if hit else "searching"
            steps.append(TraceStep(title, detail, draw_step, sum(1 for e in visible if e[0] == "node"), primitive_tests, result))

        def draw_finish(app: TraceVisualizer) -> None:
            draw_visible(app, events)
            if hit_obj:
                app.draw_hit_marker(hit_t)

        steps.append(TraceStep("BVH traversal complete", "The nearest primitive hit wins after missed 3D bounds prune subtrees.", draw_finish, sum(1 for e in events if e[0] == "node"), primitive_tests, f"closest hit {hit_obj.name if hit_obj else 'none'}"))
        return steps

    def make_tlas_blas_steps(self) -> list[TraceStep]:
        instances: list[tuple[str, tuple[SceneObject, ...], Box3, tuple[float, float, float]]] = []
        for index, (parts, offset) in enumerate(zip(LOCAL_PARTS, INSTANCE_OFFSETS), start=1):
            world_parts = tuple(SceneObject(part.name, part.box.translated(*offset), part.color) for part in parts)
            instances.append((f"Instance {index}", world_parts, scene_bounds(world_parts), offset))
        tlas_bounds = union_box(tuple(bounds for _name, _parts, bounds, _offset in instances))
        all_parts = tuple(part for _name, parts, _bounds, _offset in instances for part in parts)
        hit_obj, hit_t = self.closest_object_hit(all_parts)
        steps: list[TraceStep] = []

        def draw_instances(app: TraceVisualizer) -> None:
            app.draw_world()
            for name, parts, bounds, _offset in instances:
                app.draw_objects(parts)
                app.draw_box(bounds, "#2563eb", alpha=0.08, linewidth=1.8, label=name)
            app.draw_box(tlas_bounds, "#be123c", alpha=0.06, linewidth=2.4, label="TLAS root")
            app.draw_ray()

        steps.append(TraceStep("TLAS traversal", "Rotate the view to compare top-level instance bounds with mesh-local BLAS work.", draw_instances, 0, 0))

        def draw_root(app: TraceVisualizer) -> None:
            draw_instances(app)
            app.draw_box(tlas_bounds, "#be123c", alpha=0.14, linewidth=3.0, label="TLAS root hit")

        steps.append(TraceStep("Test TLAS root", "If this 3D box missed, every instance would be skipped.", draw_root, 1, 0))

        primitive_tests = 0
        visited = 1
        for name, parts, bounds, offset in instances:
            visited += 1
            hit, _t0, _t1 = ray_box_t(bounds)

            def draw_leaf(app: TraceVisualizer, current_name=name, current_bounds=bounds, current_hit=hit) -> None:
                draw_instances(app)
                app.draw_box(current_bounds, "#0f766e" if current_hit else "#94a3b8", alpha=0.18 if current_hit else 0.05, linewidth=3.0, label=f"{current_name} {'hit' if current_hit else 'miss'}")

            steps.append(TraceStep(f"Test {name} bound", "A TLAS leaf stores an instance transform and a BLAS pointer.", draw_leaf, visited, primitive_tests, "instance hit" if hit else "searching"))
            if not hit:
                continue

            def draw_transform(app: TraceVisualizer, current_bounds=bounds, current_name=name, current_offset=offset) -> None:
                draw_instances(app)
                app.draw_box(current_bounds, "#7c3aed", alpha=0.18, linewidth=3.0, label=current_name)
                app.ax.text(current_bounds.cx, current_bounds.cy, current_bounds.z1 + 6, f"ray -> local space\n-offset {current_offset}", color="#5b21b6", fontsize=9, ha="center")

            steps.append(TraceStep(f"Enter {name} BLAS", "The BLAS can be reused because rays are transformed into instance-local coordinates.", draw_transform, visited, primitive_tests, "inside BLAS"))

            for obj in parts:
                primitive_tests += 1
                obj_hit, _obj_t, _obj_exit = ray_box_t(obj.box)

                def draw_primitive(app: TraceVisualizer, current_obj=obj, current_hit=obj_hit) -> None:
                    draw_instances(app)
                    app.draw_box(current_obj.box, "#dc2626" if current_hit else "#64748b", alpha=0.48 if current_hit else 0.06, linewidth=3.0 if current_hit else 1.4, label=current_obj.name)

                steps.append(TraceStep(f"Test BLAS primitive {obj.name}", "Only primitives inside hit instances are tested.", draw_primitive, visited, primitive_tests, f"candidate {obj.name}" if obj_hit else "inside BLAS"))

        def draw_finish(app: TraceVisualizer) -> None:
            draw_instances(app)
            if hit_obj:
                app.draw_box(hit_obj.box, "#dc2626", alpha=0.50, linewidth=3.0, label=hit_obj.name)
                app.draw_hit_marker(hit_t)

        steps.append(TraceStep("TLAS/BLAS traversal complete", "The TLAS reduced instance work; BLAS handled local primitive tests.", draw_finish, visited, primitive_tests, f"closest hit {hit_obj.name if hit_obj else 'none'}"))
        return steps


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Visualize traversal through grid, BVH, and TLAS/BLAS acceleration structures in Matplotlib 3D.")
    parser.add_argument("--mode", choices=MODES, default="grid", help="Initial visualization mode.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    TraceVisualizer(args.mode).run()


if __name__ == "__main__":
    main()
