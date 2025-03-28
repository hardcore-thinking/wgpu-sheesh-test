struct MyUniforms {
	projectionMatrix: mat4x4f,
	viewMatrix: mat4x4f,
	modelMatrix: mat4x4f,
	color: vec4f,
	time: f32
};

@group(0) @binding(0) var<uniform> myUniforms: MyUniforms;

struct VertexInput {
	@location(0) position: vec3f,
	@location(1) color: vec3f
};

struct VertexOutput {
	@builtin(position) position: vec4f,
	@location(0) color: vec3f
};

@vertex fn vert_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput;
	out.position = myUniforms.projectionMatrix * myUniforms.viewMatrix * myUniforms.modelMatrix * vec4f(in.position, 1.0);
	out.color = in.color;
	return out;
}

@fragment fn frag_main(in: VertexOutput) -> @location(0) vec4f {
	let color = in.color * myUniforms.color.rgb;
	let linear_color = pow(color, vec3f(2.2));
	return vec4f(linear_color, myUniforms.color.a);
}