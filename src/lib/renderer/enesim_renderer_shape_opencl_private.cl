ENESIM_OPENCL_CODE(
/* Shared definitions between enesim's host code and enesim's device code
 * we MUST keep this file and enesim_renderer_shape.h in sync
 */

typedef enum _Enesim_Renderer_Shape_Fill_Rule
{
	ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO,
	ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD,
} Enesim_Renderer_Shape_Fill_Rule;
)
