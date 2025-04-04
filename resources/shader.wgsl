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
	@location(1) normal: vec3f,
	@location(2) color: vec3f,
};

struct VertexOutput {
	@builtin(position) position: vec4f,
	@location(0) color: vec3f,
	@location(1) normal: vec3f,
};

@vertex fn vert_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput;
	out.position = myUniforms.projectionMatrix * myUniforms.viewMatrix * myUniforms.modelMatrix * vec4f(in.position, 1.0);
	out.normal = (myUniforms.modelMatrix * vec4f(in.normal, 0.0)).xyz;
	out.color = in.color;
	return out;
}

@fragment fn frag_main(in: VertexOutput) -> @location(0) vec4f {
	let normal = normalize(in.normal);
	let lightColor1 = vec3f(1.0f, 0.9, 0.6);
	let lightColor2 = vec3f(0.6f, 0.9, 1.0);
	let lightDirection1 = vec3f(0.5, -0.9, 0.1);
	let lightDirection2 = vec3f(0.2, 0.4, 0.3);
	let shading1 = max(0.0, dot(lightDirection1, normal));
	let shading2 = max(0.0, dot(lightDirection2, normal));
	let shading = shading1 * lightColor1 + shading2 * lightColor2;
	let color = in.color * shading;
	//let color = in.color * -normal.z;
	//let color = in.color * myUniforms.color.rgb;
	let linear_color = pow(color, vec3f(2.2));
	return vec4f(linear_color, myUniforms.color.a);
}