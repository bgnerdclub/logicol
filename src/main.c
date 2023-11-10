#include <stdio.h>
#include <stdint.h>
#include "raylib.h"
#include "math.h"
#include "memory.h"
#include "stdlib.h"

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64; 
typedef float    f32;
typedef double   f64;
typedef size_t   usize;

#define PURPLE (Color){ 126, 34, 206, 255 }
#define BACKGROUND (Color){ 15, 23, 42, 255 }

static f32 FONT_SIZE = 48.0;
static f32 FONT_SPACING = 8.0;

typedef struct Component Component;
typedef usize ComponentRef;

typedef struct {
	usize outputIndex;
	ComponentRef component;
} Input;

typedef struct Component {
	Vector2 pos;
	usize id;
	const char* name;
	usize numInputs;
	Input* inputs;
	usize numOutputs;
	bool* outputs;
	bool ticked;
} Component;

typedef struct {
	usize numComponents;
	Component* components;
	usize lastID;
} Circuit;

Circuit createCircuit() {
	Circuit circuit;
	memset(&circuit, 0, sizeof circuit);
	return circuit;
}

Component* getComponent(Circuit* circuit, ComponentRef ref) {
	return &circuit->components[ref - 1];
}

ComponentRef addComponent(Circuit* circuit, const char* name, usize numInputs, usize numOutputs) {
	Component component;
	component.pos = (Vector2){ 0, 0 };
	component.id = ++circuit->lastID;
	printf("%zu\n", component.id);
	component.name = name;
	component.numInputs = numInputs;
	component.inputs = malloc(sizeof *component.inputs * numInputs);
	memset(component.inputs, 0, sizeof *component.inputs * numInputs);
	component.numOutputs = numOutputs;
	component.outputs = malloc(sizeof(bool) * component.numOutputs);
	memset(component.outputs, 0, sizeof(bool) * component.numOutputs);
	component.ticked = false;

	circuit->numComponents++;
	circuit->components = realloc(circuit->components, circuit->numComponents * sizeof *circuit->components);
	circuit->components[circuit->numComponents - 1] = component;
	return component.id;
}

void addConnection(Circuit* circuit, ComponentRef from, ComponentRef to, usize fromIndex, usize toIndex) {
	Component* t = getComponent(circuit, to);	
	Component* f = getComponent(circuit, from);

	t->inputs[toIndex].component = from;
	t->inputs[toIndex].outputIndex = fromIndex;
}

Vector2 getSize(Component* component) {
	Vector2 textSize = MeasureTextEx(GetFontDefault(), component->name, FONT_SIZE, FONT_SPACING);

	char idStr[(int)((ceil(log10(component->id))+1)*sizeof(char))];
	sprintf(idStr, "%zu", component->id);
	Vector2 idSize = MeasureTextEx(GetFontDefault(), idStr, FONT_SIZE, FONT_SPACING);
	
	return (Vector2){ textSize.x + 32.0, textSize.y + idSize.y + 24.0 };
}

Vector2 getInputPos(Component* component, usize input) {
	Vector2 rect = getSize(component);
	Vector2 start = { component->pos.x, component->pos.y + (rect.y * (input + 1))/(component->numInputs + 1) };
	return (Vector2){ start.x - 32, start.y };
}

Vector2 getOutputPos(Component* component, usize output) {
	Vector2 rect = getSize(component);
	Vector2 start = { component->pos.x + rect.x, component->pos.y + (rect.y * (output + 1))/(component->numOutputs + 1) };
	return (Vector2){ start.x + 32, start.y };
}

void drawComponent(Circuit* circuit, Component* component) {
	f32 x = component->pos.x;
	f32 y = component->pos.y;

	Vector2 textSize = MeasureTextEx(GetFontDefault(), component->name, FONT_SIZE, FONT_SPACING);

	char idStr[(int)((ceil(log10(component->id))+1)*sizeof(char))];
	sprintf(idStr, "%zu", component->id);
	Vector2 idSize = MeasureTextEx(GetFontDefault(), idStr, FONT_SIZE, FONT_SPACING);
	
	Rectangle rect = { x, y, textSize.x + 32.0, textSize.y + idSize.y + 24.0 };

	Color color = PURPLE;
	if (strcmp(component->name, "INPUT") == 0) {
		if (component->outputs[0]) {
			color = GREEN;
		} else {
			color = RED;
		}
	}

	DrawRectangleLinesEx(rect, 6.0, color);
	DrawTextEx(GetFontDefault(), idStr, (Vector2){ x + ((rect.width - idSize.x) / 2), y + 8 }, FONT_SIZE, FONT_SPACING, color);
	DrawTextEx(GetFontDefault(), component->name, (Vector2){x + 16, y + idSize.y + 16.0 }, FONT_SIZE, FONT_SPACING, color);
	
	for (usize i = 0; i < component->numInputs; i++) {
		Vector2 start = { x, y + (rect.height * (i + 1))/(component->numInputs + 1) };
		Vector2 end = { start.x - 32, start.y };
		DrawLineEx(start, end, 6.0, PURPLE);


		if (component->inputs[i].component != 0) {
			Component* connected = getComponent(circuit, component->inputs[i].component);
			Vector2 connectedSize = getSize(connected);
			Vector2 owo = (Vector2){ 
				connected->pos.x + connectedSize.x + 32, 
				connected->pos.y + connectedSize.y * (f32)(component->inputs[i].outputIndex + 1.0)/(f32)(connected->numOutputs + 1) };
			DrawLineBezier(end, owo, 6.0, PURPLE);
		}

	}

	for (usize i = 0; i < component->numOutputs; i++) {
		Vector2 start = { x + rect.width, y + (rect.height * (i + 1))/(component->numOutputs + 1) };
		DrawLineEx(start, (Vector2){ start.x + 32, start.y }, 6.0, PURPLE);
	}
}

void drawCircuit(Circuit* circuit) {
	for (usize i = 0; i < circuit->numComponents; i++) {
		drawComponent(circuit, &circuit->components[i]);
	}
}

f32 distanceBetween(Vector2 a, Vector2 b) {
	return sqrt((a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y));
}

void moveComponent(Circuit* circuit) {
	static ComponentRef moving = 0;

	Vector2 mousePos = GetMousePosition();

	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		for (usize i = 0; i < circuit->numComponents; i++) {
			if (distanceBetween(mousePos, circuit->components[i].pos) < 25.0) {
				moving = circuit->components[i].id;
			}
		}
	}

	if (moving && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
		Component* component = getComponent(circuit, moving);
		Vector2 mouseDelta = GetMouseDelta();
		component->pos.x += mouseDelta.x;
		component->pos.y += mouseDelta.y;
	}

	if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
		moving = 0;
	}
}

void createConnection(Circuit* circuit) {
	static ComponentRef from = 0;
	static usize output = 0;

	Vector2 mousePos = GetMousePosition();

	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		for (usize i = 0; i < circuit->numComponents; i++) {
			for (usize j = 0; j < circuit->components[i].numOutputs; j++) {
				Vector2 outputPos = getOutputPos(&circuit->components[i], j);
				if (distanceBetween(mousePos, outputPos) < 25.0) {
					from = circuit->components[i].id;
					output = j;
				}
			}
		}
	}

	if (from && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
		DrawLineBezier(getOutputPos(getComponent(circuit, from), output), mousePos, 6.0, PURPLE);
	}

	if (from && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
		for (usize i = 0; i < circuit->numComponents; i++) {
			for (usize j = 0; j < circuit->components[i].numInputs; j++) {
				Vector2 inputPos = getInputPos(&circuit->components[i], j);
				if (distanceBetween(mousePos, inputPos) < 25.0) {
					addConnection(circuit, from, circuit->components[i].id, output, j);
					from = 0;
					output = 0;
				}
			}
		}
	}
}

void toggleInput(Circuit* circuit) {
	if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
		for (usize i = 0; i < circuit->numComponents; i++) {
			if (strcmp(circuit->components[i].name, "INPUT") == 0) {
				if (distanceBetween(GetMousePosition(), circuit->components[i].pos) < 25.0) {
					circuit->components[i].outputs[0] = !circuit->components[i].outputs[0];
				}
			}
		} 
	}
}

bool simulateComponent(Circuit* circuit, Input* input) {
	if (input->component == 0) return false;

	Component* component = getComponent(circuit, input->component);
	if (component->ticked) { return component->outputs[input->outputIndex]; }

	component->ticked = true;

	if (strcmp(component->name, "AND") == 0) {
		bool a = simulateComponent(circuit, &component->inputs[0]);
		bool b = simulateComponent(circuit, &component->inputs[1]);
		component->outputs[0] = a & b;
	} else if (strcmp(component->name, "OR") == 0) {
		bool a = simulateComponent(circuit, &component->inputs[0]);
		bool b = simulateComponent(circuit, &component->inputs[1]);
		component->outputs[0] = a | b;
	} else if (strcmp(component->name, "XOR") == 0) {
		bool a = simulateComponent(circuit, &component->inputs[0]);
		bool b = simulateComponent(circuit, &component->inputs[1]);
		component->outputs[0] = a ^ b;
	} else if (strcmp(component->name, "NOT") == 0) {
		bool a = simulateComponent(circuit, &component->inputs[0]);
		component->outputs[0] = !a;
	} else if (strcmp(component->name, "INPUT") == 0) {}

	return component->outputs[input->outputIndex];
}

void simulate(Circuit* circuit) {
	for (usize i = 0; i < circuit->numComponents; i++) {
		circuit->components[i].ticked = false;
	}

	for (usize i = 0; i < circuit->numComponents; i++) {
		if (strcmp(circuit->components[i].name, "OUTPUT") == 0) {
			printf("%d", simulateComponent(
				circuit, 
				&circuit->components[i].inputs[0]
			));
		}
	}

	printf("\n");
}

int logicol_main() {
	InitWindow(640, 480, "Logicol");
	SetTargetFPS(60);

	Circuit circuit = createCircuit();

	while (!WindowShouldClose()) {
		BeginDrawing();
		{
			drawCircuit(&circuit);
			ClearBackground(BACKGROUND);

			if (IsKeyPressed(KEY_A)) {
				addComponent(&circuit, "AND", 2, 1);
			}

			if (IsKeyPressed(KEY_O)) {
				addComponent(&circuit, "OR", 2, 1);
			}

			if (IsKeyPressed(KEY_X)) {
				addComponent(&circuit, "XOR", 2, 1);
			}

			if (IsKeyPressed(KEY_N)) {
				addComponent(&circuit, "NOT", 1, 1);
			}

			if (IsKeyPressed(KEY_I)) {
				addComponent(&circuit, "INPUT", 0, 1);
			}

			if (IsKeyPressed(KEY_U)) {
				addComponent(&circuit, "OUTPUT", 1, 0);
			}

			moveComponent(&circuit);
			createConnection(&circuit);
			toggleInput(&circuit);

			if (IsKeyPressed(KEY_SPACE)) {
				simulate(&circuit);
			}
		}
		EndDrawing();
	}

	return 0;
}

