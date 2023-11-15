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

typedef struct {
  usize length;
  char* data;
} String;

String createString() {
  String string;
  string.length = 0;
  string.data = NULL;
  return string;
}

void destroyString(String* string) {
  free(string->data);
}

String fromCString(const char* data) {
  String string;
  string.length = strlen(data);
  string.data = malloc(sizeof(char) * string.length);
  memcpy(string.data, data, sizeof(char) * string.length);
  return string;
}

void appendString(String* base, String* extra) {
  usize old = base->length;
  base->length += extra->length;
  base->data = realloc(base->data, sizeof(char) * base->length);
  memcpy(&base->data[old], extra->data, sizeof(char) * extra->length);
}

bool stringEqual(String* a, String* b) {
  if (a->length != b->length) return false;

  for (usize i = 0; i < a->length; i++) {
    if (a->data[i] != b->data[i]) return false;
  }

  return true;
}

char* toCString(String* string) {
  char* buffer = malloc(sizeof(char) * (string->length + 1));
  memset(buffer, 0, sizeof(char) * (string->length + 1));
  memcpy(buffer, string->data, sizeof(char) * string->length);
  return buffer;
}

String cloneString(String* base) {
  String string;
  string.length = base->length;
  string.data = malloc(sizeof(char) * base->length);
  memcpy(string.data, base->data, sizeof(char) * base->length);
  return string;
}

#define PURPLE (Color){ 126, 34, 206, 255 }
#define BACKGROUND (Color){ 15, 23, 42, 255 }

static f32 FONT_SIZE = 48.0;
static f32 FONT_SPACING = 8.0;

typedef struct Component Component;
typedef usize ComponentRef;
typedef usize CircuitRef;

typedef struct {
	usize outputIndex;
	ComponentRef component;
} Input;

typedef struct Component {
	ComponentRef id;
	Vector2 pos;
	String name;
	usize numInputs;
	Input* inputs;
	usize numOutputs;
	bool* outputs;
	bool ticked;
} Component;

typedef struct {
  CircuitRef id;
	usize numComponents;
	Component* components;
  String name;
} Circuit;

typedef struct {
  usize numCircuits;
  Circuit* circuits;
} Project;

Project createProject() {
  Project project;
  project.numCircuits = 0;
  project.circuits = NULL;
  return project;
}

CircuitRef addCircuit(Project* project, String name) {
  project->numCircuits++;
  project->circuits = realloc(project->circuits, project->numCircuits * sizeof *project->circuits);
	Circuit* circuit = &project->circuits[project->numCircuits - 1];
  circuit->id = project->numCircuits;
  circuit->numComponents = 0;
  circuit->components = NULL;
  circuit->name = name;
  printf("%d\n", name.length);
  printf("%d\n", circuit->name.length);
	return circuit->id;
}

Circuit* getCircuit(Project* project, CircuitRef ref) {
  return &project->circuits[ref - 1];
}

Component* getComponent(Circuit* circuit, ComponentRef ref) {
	return &circuit->components[ref - 1];
}

ComponentRef addComponent(Circuit* circuit, String name, usize numInputs, usize numOutputs) {
	Component component;
	component.pos = (Vector2){ 100, 100 };
	component.id = circuit->numComponents + 1;
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

  if (t->inputs[toIndex].component == from && t->inputs[toIndex].outputIndex == fromIndex) { // Delete connection
    t->inputs[toIndex].component = 0;
	  t->inputs[toIndex].outputIndex = 0;
    return;
  }

	t->inputs[toIndex].component = from;
	t->inputs[toIndex].outputIndex = fromIndex;
}

Vector2 getSize(Component* component) {
  char* buffer = toCString(&component->name);
	Vector2 textSize = MeasureTextEx(GetFontDefault(), buffer, FONT_SIZE, FONT_SPACING);
  free(buffer);

	char idStr[(int)((ceil(log10(component->id))+1)*sizeof(char))];
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

  char* buffer = toCString(&component->name);
	Vector2 textSize = MeasureTextEx(GetFontDefault(), buffer, FONT_SIZE, FONT_SPACING);
  free(buffer);

	char idStr[(int)((ceil(log10(component->id))+1)*sizeof(char))];
	sprintf(idStr, "%zu", component->id);
	Vector2 idSize = MeasureTextEx(GetFontDefault(), idStr, FONT_SIZE, FONT_SPACING);
	
	Rectangle rect = { x, y, textSize.x + 32.0, textSize.y + idSize.y + 24.0 };

	Color color = PURPLE;
  String input = fromCString("INPUT");
  String output = fromCString("OUTPUT");
	if (stringEqual(&component->name, &input)) {
		if (component->outputs[0]) {
			color = GREEN;
		} else {
			color = RED;
		}
	}
  if (stringEqual(&component->name, &output)) {
    if (component->inputs[0].component != 0 && getComponent(circuit, component->inputs[0].component)->outputs[component->inputs[0].outputIndex]) {
			color = GREEN;
		} else {
			color = RED;
		}
  }
  destroyString(&input);
  destroyString(&output);

	DrawRectangleLinesEx(rect, 6.0, color);
	DrawTextEx(GetFontDefault(), idStr, (Vector2){ x + ((rect.width - idSize.x) / 2), y + 8 }, FONT_SIZE, FONT_SPACING, color);

  buffer = toCString(&component->name);
	DrawTextEx(GetFontDefault(), buffer, (Vector2){x + 16, y + idSize.y + 16.0 }, FONT_SIZE, FONT_SPACING, color);
  free(buffer);
	
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
      Color connectionColor = RED;
      if (connected->outputs[component->inputs[i].outputIndex]) { connectionColor = GREEN; }
			DrawLineBezier(end, owo, 6.0, connectionColor);
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

void moveComponent(Circuit* circuit, Camera2D camera) {
	static ComponentRef moving = 0;

	Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);

	if (!IsKeyDown(KEY_LEFT_CONTROL) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		for (usize i = 0; i < circuit->numComponents; i++) {
			if (distanceBetween(mousePos, circuit->components[i].pos) < 25.0) {
				moving = circuit->components[i].id;
			}
		}
	}

	if (moving && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
		Component* component = getComponent(circuit, moving);
		Vector2 mouseDelta = GetMouseDelta();
		component->pos.x += mouseDelta.x / camera.zoom;
		component->pos.y += mouseDelta.y / camera.zoom;
	}

	if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
		moving = 0;
	}
}

void createConnection(Circuit* circuit, Camera2D camera) {
	static ComponentRef from = 0;
	static usize output = 0;

	Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);

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

void toggleInput(Circuit* circuit, Camera2D camera) {
  Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);

  String input = fromCString("INPUT");

	if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
		for (usize i = 0; i < circuit->numComponents; i++) {
			if (stringEqual(&circuit->components[i].name, &input)) {
				if (distanceBetween(mousePos, circuit->components[i].pos) < 25.0) {
					circuit->components[i].outputs[0] = !circuit->components[i].outputs[0];
				}
			}
		} 
	}

  destroyString(&input);
}

void moveCamera(Camera2D* camera) {
  if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
    camera->target.x -= GetMouseDelta().x / camera->zoom;
    camera->target.y -= GetMouseDelta().y / camera->zoom;
  }

  float oldWidth = (float)GetScreenWidth() / camera->zoom;
  float oldHeight = (float)GetScreenHeight() / camera->zoom;

  camera->zoom *= powf(1.1, GetMouseWheelMove());

  float newWidth = (float)GetScreenWidth() / camera->zoom;
  float newHeight = (float)GetScreenHeight() / camera->zoom;

  camera->target.x += (oldWidth - newWidth) / 2;
  camera->target.y += (oldHeight - newHeight) / 2;
}

CircuitRef getActive(Project* project, Circuit* active) {
  static usize numPrevious = 0;
  static CircuitRef* previous = NULL;

  if (IsKeyDown(KEY_LEFT_CONTROL) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    for (usize i = 0; i < active->numComponents; i++) {
      if (distanceBetween(GetMousePosition(), active->components[i].pos) < 25.0) {
        for (usize j = 0; j < project->numCircuits; j++) {
          if (stringEqual(&project->circuits[j].name, &active->components[i].name)) {
            numPrevious++;
            previous = realloc(previous, numPrevious * sizeof *previous);
            previous[numPrevious - 1] = active->id;
            return j + 1;
          }
        }     
      }
    }
  }

  if (IsKeyPressed(KEY_BACKSPACE)) {
    if (numPrevious > 0) {
      numPrevious--;
      return previous[numPrevious];
    }
  }

  return active->id;
}

void drawActive(Circuit* circuit) {
  String editing = fromCString("Editing: ");
  appendString(&editing, &circuit->name);
  char* buffer = toCString(&editing);
  printf("%s\n", toCString(&circuit->name));
  DrawTextEx(GetFontDefault(), buffer, (Vector2){ 16.0, 16.0 }, FONT_SIZE, FONT_SPACING, PURPLE);
  free(buffer);
  destroyString(&editing);
}

void updateInputs(Project* project, Circuit* active) {
  usize numInputs = 0;
  String input = fromCString("INPUT");
  for (usize i = 0; i < active->numComponents; i++) {
    if (stringEqual(&active->components[i].name, &input)) {
      numInputs++;
    }
  }
  destroyString(&input);

  for (usize i = 0; i < project->numCircuits; i++) {
    for (usize j = 0; j < project->circuits[i].numComponents; j++) {
      if (stringEqual(&project->circuits[i].components[j].name, &active->name)) {
        project->circuits[i].components[j].numInputs = numInputs;
        project->circuits[i].components[j].inputs = realloc(project->circuits[i].components[j].inputs, sizeof(Input) * numInputs);
        memset(&project->circuits[i].components[j].inputs[numInputs - 1], 0, sizeof(Input));
      }
    }
  }
}

void updateOutputs(Project* project, Circuit* active) {
  usize numOutputs = 0;
  String output = fromCString("OUTPUT");
  for (usize i = 0; i < active->numComponents; i++) {
    if (stringEqual(&active->components[i].name, &output)) {
      numOutputs++;
    }
  }
  destroyString(&output);

  for (usize i = 0; i < project->numCircuits; i++) {
    for (usize j = 0; j < project->circuits[i].numComponents; j++) {
      if (stringEqual(&project->circuits[i].components[j].name, &active->name)) {
        project->circuits[i].components[j].numOutputs = numOutputs;
        project->circuits[i].components[j].outputs = realloc(project->circuits[i].components[j].outputs, sizeof(bool) * numOutputs);
        project->circuits[i].components[j].outputs[numOutputs - 1] = false;
      }
    }
  }
}

typedef enum NodeType {
  NAND = 0, 
  INPUT, 
} NodeType;
typedef struct Node Node;
typedef struct Node {
  Node* left;
  Node* right;
  bool ticked;
  bool output;
  NodeType type;
} Node;

typedef struct {
  usize numRoots;
  Node* roots;
} Tree;

void resetNodes(Node* node) {
  node->ticked = false;
  if (node->left) resetNodes(node->left);
  if (node->right) resetNodes(node->left);
}

bool tickNode(Node* node) {
  if (!node) return false;
  if (node->ticked) return node->output;
  node->ticked = true;

  if (node->type == NAND) {
    if (!node->left || !node->right) printf("PANIC\n");
    node->output = !(tickNode(node->left) && tickNode(node->right));
  }

  return node->output;
}

Node* createNode() {
  Node* node = malloc(sizeof(Node));
  memset(node, 0, sizeof(Node));
  return node;
}

typedef struct Parent Parent;
typedef struct Parent {
  Component* component;
  Circuit* circuit;
  Parent* parent;
  Circuit* root;
} Parent;

void compileComponent(Node* node, Project* project, Circuit* circuit, Input* input, Parent* parent) {
  memset(node, 0, sizeof(Node));
  if (input->component == 0) return;
  Component* component = getComponent(circuit, input->component);

  String and = fromCString("AND");
  String or = fromCString("OR");
  String not = fromCString("NOT");
  String inputStr = fromCString("INPUT");
  String output = fromCString("OUTPUT");

  if (stringEqual(&component->name, &and)) {
    Node* child = createNode();
    node->left = child;
    node->right = child;
    child->left = createNode();
    compileComponent(child->left, project, circuit, &component->inputs[0], parent); 
    child->right = createNode();
    compileComponent(child->right, project, circuit, &component->inputs[1], parent); 
  } else if (stringEqual(&component->name, &or)) {
    node->left = createNode();
    node->right = createNode();
    Node* left = createNode();
    node->left->left = left;
    node->left->right = left;
    compileComponent(left, project, circuit, &component->inputs[0], parent); 
    Node* right = createNode();
    node->right->left = right;
    node->right->right = right;
    compileComponent(right, project, circuit, &component->inputs[1], parent); 
  } else if (stringEqual(&component->name, &not)) {
    Node* child =  createNode();
    node->left = child;
    node->right = child;
    compileComponent(child, project, circuit, &component->inputs[0], parent);
  } else if (stringEqual(&component->name, &inputStr)) {
    if (circuit == parent->root) {
      node->type = INPUT;
      node->output = component->outputs[0];
    } else {
      usize numInput = 0;
      for (usize i = 0; i < circuit->numComponents; i++) {
        if (stringEqual(&circuit->components[i].name, &inputStr)) {
          if (circuit->components[i].id == input->component) break;
          numInput++;
        }
      }
      printf("Hit %zu input", numInput);

      compileComponent(node, project, parent->circuit, &parent->component->inputs[numInput], parent->parent);
    }
  } else {
    for (usize i = 0; i < project->numCircuits; i++) {
      if (stringEqual(&component->name, &project->circuits[i].name)) {
        usize counter = 0;
        for (usize j = 0; j < project->circuits[i].numComponents; j++) {
          if (stringEqual(&project->circuits[i].components[j].name, &output)) {
            if (counter++ == input->outputIndex) {
              Parent p;
              p.component = component;
              p.circuit = circuit;
              p.parent = parent;
              p.root = parent->root;
              compileComponent(node, project, &project->circuits[i], &project->circuits[i].components[j].inputs[0], &p);
            }
          }
        }
      }
    }
  }

  destroyString(&and);
  destroyString(&or);
  destroyString(&not);
  destroyString(&inputStr);
  destroyString(&output);
}

Tree compileProject(Project* project, Circuit* root) {
  String output = fromCString("OUTPUT");

  usize numRoots = 0;
  for (usize i = 0; i < root->numComponents; i++) {
    if (stringEqual(&root->components[i].name, &output)) {
      numRoots++;
    }
  }

  Tree tree;
  tree.numRoots = numRoots;
  tree.roots = malloc(sizeof(Node) * numRoots);

  for (usize i = 0; i < root->numComponents; i++) {
    if (stringEqual(&root->components[i].name, &output)) {
      Parent parent;
      memset(&parent, 0, sizeof(Parent));
      parent.root = root;
      compileComponent(&tree.roots[--numRoots], project, root, &root->components[i].inputs[0], &parent);
    }
  }

  destroyString(&output);

  return tree;
}

void tickTree(Tree tree) {
  for (usize i = 0; i < tree.numRoots; i++) {
     resetNodes(&tree.roots[i]);
  }

  for (usize i = 0; i < tree.numRoots; i++) {
    printf("%d\n", tickNode(&tree.roots[i]));
  }
}

usize getComponentSize(Component* component) {
  usize size = 0;
  size += sizeof(component->id);
  size += sizeof(component->pos);
  size += sizeof(component->name.length);
  size += component->name.length;
  size += sizeof(component->numInputs);
  size += sizeof(Input) * component->numInputs;
  size += sizeof(component->numOutputs);
  size += sizeof(bool) * component->numOutputs;
  return size;
}

usize getCircuitSize(Circuit* circuit) {
  usize size = 0;
  size += sizeof(circuit->id);
  size += sizeof(circuit->name.length);
  size += circuit->name.length;
  size += sizeof(circuit->numComponents);
  for (usize i = 0; i < circuit->numComponents; i++) {
    size += getComponentSize(&circuit->components[i]);
  }
  return size;
}

#define PUT(thing) memcpy(&buffer[pointer], &thing, sizeof(thing)); pointer += sizeof(thing)

void saveComponent(Component* component, char* buffer) {
  usize pointer = 0;
  PUT(component->id);
  PUT(component->pos);
  PUT(component->name.length);
  memcpy(&buffer[pointer], component->name.data, component->name.length);
  pointer += component->name.length;
  PUT(component->numInputs);
  memcpy(&buffer[pointer], component->inputs, sizeof(Input) * component->numInputs);
  pointer += sizeof(Input) * component->numInputs;
  PUT(component->numOutputs);
  memcpy(&buffer[pointer], component->outputs, sizeof(bool) * component->numOutputs);
  pointer += sizeof(bool) * component->numOutputs;
}

void saveCircuit(Circuit* circuit, char* buffer) {
  usize pointer = 0;
  PUT(circuit->id);
  PUT(circuit->name.length);
  memcpy(&buffer[pointer], circuit->name.data, circuit->name.length);
  pointer += circuit->name.length;
  PUT(circuit->numComponents);
  for (usize i = 0; i < circuit->numComponents; i++) {
    saveComponent(&circuit->components[i], &buffer[pointer]);
    pointer += getComponentSize(&circuit->components[i]);
  }
}

void saveProject(Project* project) {
  usize size = 0;
  size += 8; // project->numCircuits
  for (usize i = 0; i < project->numCircuits; i++) {
    size += getCircuitSize(&project->circuits[i]);
  }
  printf("Size: %d\n", size);

  char* buffer = malloc(size);
  memcpy(&buffer[0], &project->numCircuits, sizeof(usize));
  usize pointer = sizeof(usize);
  for (usize i = 0; i < project->numCircuits; i++) {
    saveCircuit(&project->circuits[i], &buffer[pointer]);
    pointer += getCircuitSize(&project->circuits[i]);
  }

  FILE* file = fopen("test.logic", "wb+");
  fwrite(buffer, size, 1, file);
  fclose(file);

  free(buffer);
}

#define GET(thing) memcpy(&thing, &buffer[pointer], sizeof(thing)); pointer += sizeof(thing)

usize loadComponent(Component* component, char* buffer) {
  usize pointer = 0;
  GET(component->id);
  GET(component->pos);
  GET(component->name.length);
  component->name.data = malloc(component->name.length);
  memcpy(component->name.data, &buffer[pointer], component->name.length);
  pointer += component->name.length;
  GET(component->numInputs);
  component->inputs = malloc(sizeof(Input) * component->numInputs);
  memcpy(component->inputs, &buffer[pointer], sizeof(Input) * component->numInputs);
  pointer += sizeof(Input) * component->numInputs;
  GET(component->numOutputs);
  component->outputs = malloc(sizeof(bool) * component->numOutputs);
  memcpy(component->outputs, &buffer[pointer], sizeof(bool) * component->numOutputs);
  pointer += sizeof(bool) * component->numOutputs;
  return pointer;
}

usize loadCircuit(Circuit* circuit, char* buffer) {
  usize pointer = 0;
  GET(circuit->id);
  GET(circuit->name.length);
  circuit->name.data = malloc(circuit->name.length);
  memcpy(circuit->name.data, &buffer[pointer], circuit->name.length);
  pointer += circuit->name.length;
  GET(circuit->numComponents);
  circuit->components = malloc(sizeof(Component) * circuit->numComponents);
  memset(circuit->components, 0, sizeof(Component) * circuit->numComponents);
  for (usize i = 0; i < circuit->numComponents; i++) {
    pointer += loadComponent(&circuit->components[i], &buffer[pointer]);
  }
  return pointer;
}

Project loadProject() {
  FILE* file = fopen("test.logic", "rb");
  fseek(file, 0, SEEK_END);
  usize size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char* buffer = malloc(size);
  fread(buffer, size, 1, file);
  usize pointer = 0;
  
  Project project;
  GET(project.numCircuits);
  printf("numCircuits: %d\n", project.numCircuits);
  project.circuits = malloc(sizeof(Circuit) * project.numCircuits);
  memset(project.circuits, 0, sizeof(Circuit) * project.numCircuits);
  for (usize i = 0; i < project.numCircuits; i++) {
    pointer += loadCircuit(&project.circuits[i], &buffer[pointer]);
  }
  printf("Pointer: %zu\n", pointer);
  return project;
}

int logicol_main() {
	InitWindow(640, 480, "Logicol");
	SetTargetFPS(60);

  Project project = createProject();
  CircuitRef active = addCircuit(&project, fromCString("MAIN"));
  printf("%d\n", project.circuits[0].name.length);
  Camera2D camera = { 0 };
  camera.zoom = 1.0;

  bool inputting = false;
  String buffer = createString();

	while (!WindowShouldClose()) {
    Circuit* circuit = getCircuit(&project, active);

    BeginDrawing();
    {
      ClearBackground(BACKGROUND);
      drawActive(circuit);
    }

		BeginMode2D(camera);
		{
			drawCircuit(circuit);

			if (!inputting && IsKeyPressed(KEY_A)) {
				addComponent(circuit, fromCString("AND"), 2, 1);
			}

			if (!inputting && IsKeyPressed(KEY_O)) {
				addComponent(circuit, fromCString("OR"), 2, 1);
			}

			if (!inputting && IsKeyPressed(KEY_X)) {
				addComponent(circuit, fromCString("XOR"), 2, 1);
			}

			if (!inputting && IsKeyPressed(KEY_N)) {
				addComponent(circuit, fromCString("NOT"), 1, 1);
			}

			if (!inputting && IsKeyPressed(KEY_I)) {
				addComponent(circuit, fromCString("INPUT"), 0, 1);
        updateInputs(&project, circuit);
			}

			if (!inputting && IsKeyPressed(KEY_U)) {
				addComponent(circuit, fromCString("OUTPUT"), 1, 0);
        updateOutputs(&project, circuit);
			}

      if (inputting && IsKeyPressed(KEY_ENTER)) {
        inputting = false;
        CircuitRef found = 0;
        for (usize i = 0; i < project.numCircuits; i++) {
          if (stringEqual(&buffer, &project.circuits[i].name)) {
            found = project.circuits[i].id;
            break;
          }
        }

        if (!found) {
          found = addCircuit(&project, cloneString(&buffer));
          circuit = getCircuit(&project, active);
        }

        Circuit* child = getCircuit(&project, found);
        usize numInputs = 0;
        usize numOutputs = 0;
        String inputStr = fromCString("INPUT"); 
        String outputStr = fromCString("OUTPUT"); 
        for (usize j = 0; j < child->numComponents; j++) {
          if (stringEqual(&child->components[j].name, &inputStr)) numInputs++;
          if (stringEqual(&child->components[j].name, &outputStr)) numOutputs++;
        }

        addComponent(circuit, buffer, numInputs, numOutputs);

        destroyString(&inputStr);
        destroyString(&outputStr);
        buffer = createString();
      }

      if (inputting) {
        int character;
        while ((character = GetCharPressed()) != 0) {
          buffer.length++;
          buffer.data = realloc(buffer.data, sizeof(char) * buffer.length);
          buffer.data[buffer.length - 1] = character;
        }
        DrawRectangleV((Vector2){ 0, GetScreenHeight() - 64 }, (Vector2){ GetScreenWidth(), GetScreenHeight() }, PURPLE);
        char* b = toCString(&buffer);
        DrawTextEx(GetFontDefault(), b, (Vector2){ 16, GetScreenHeight() - 56 }, FONT_SIZE, FONT_SPACING, BACKGROUND);
        free(b);
      }

      if (!inputting && IsKeyPressed(KEY_C)) {
        inputting = true;
      }

			moveComponent(circuit, camera);
			createConnection(circuit, camera);
			toggleInput(circuit, camera);
      moveCamera(&camera);

      if (IsKeyPressed(KEY_S)) {
        saveProject(&project);
      }

      if (IsKeyPressed(KEY_L)) {
        project = loadProject();
        active = project.circuits[0].id;
      }
      
      if (IsKeyPressed(KEY_SPACE)) {
        Tree tree = compileProject(&project, circuit);
        tickTree(tree);
        
        usize counter = tree.numRoots;;
        String outputStr = fromCString("OUTPUT");
        for (usize i = 0; i < circuit->numComponents; i++) {
          if (stringEqual(&circuit->components[i].name, &outputStr)) {
            Input* input = &circuit->components[i].inputs[0];
            getComponent(circuit, input->component)->outputs[input->outputIndex] = tree.roots[--counter].output;
          }
        }
        destroyString(&outputStr);
        saveProject(&project);
      }

      active = getActive(&project, circuit);
		}
    EndMode2D();
		EndDrawing();
	}

	return 0;
}

