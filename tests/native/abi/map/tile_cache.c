// mln_map_options.tile_cache_size sets how many tiles each source keeps after
// they leave the view. Each case renders a still image of one view, one of a
// view elsewhere, and one of the first view again, and counts the tile requests
// that the last image loads: a tile the source kept is only revalidated.
//
// Every tile is the same recorded MLT tile. A request that carries the expiry
// of an earlier response is a revalidation, which the provider answers as not
// modified and does not count. The runtime's ambient cache is off, so a tile
// the source dropped reaches the provider again as a load.

#include "support/map.h"
#include "support/resources.h"
#include "support/test_support.h"

static const char tile_cache_style_json[] =
  "{\"version\":8,\"name\":\"tile-cache\",\"sources\":{\"tiles\":{"
  "\"type\":\"vector\",\"encoding\":\"mlt\","
  "\"tiles\":[\"custom://tiles/{z}/{x}/{y}.mlt\"],"
  "\"minzoom\":0,\"maxzoom\":14}},\"layers\":[{\"id\":\"admin-lines\","
  "\"type\":\"line\",\"source\":\"tiles\",\"source-layer\":\"admin\"}]}";

typedef struct tile_server {
  uint8_t* bytes;
  size_t byte_count;
  atomic_int tile_loads;
} tile_server;

static void serve_tile(
  void* user_data, const mln_resource_request_info* request,
  mln_resource_request handle
) {
  tile_server* server = user_data;
  const bool revalidation =
    (request->fields & MLN_RESOURCE_REQUEST_PRIOR_EXPIRES) != 0U;
  if (request->kind == MLN_RESOURCE_KIND_TILE && !revalidation) {
    atomic_fetch_add(&server->tile_loads, 1);
  }
  const mln_resource_response response = {
    .size = sizeof(mln_resource_response),
    .fields = MLN_RESOURCE_RESPONSE_EXPIRES,
    .status = revalidation ? MLN_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED
                           : MLN_RESOURCE_RESPONSE_STATUS_OK,
    .error_reason = MLN_RESOURCE_ERROR_REASON_NONE,
    .bytes = server->bytes,
    .byte_count = server->byte_count,
    .expires_unix_ms = 4102444800000,
  };
  mln_test_request_answer(handle, &response);
}

static void render_at(
  const mln_test_render_fixture* fixture, mln_map map, double latitude,
  double longitude
) {
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  update.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  update.camera.center = (mln_lat_lng){latitude, longitude};
  update.camera.zoom = 10.0;
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
  MLN_TEST_OK(mln_test_render_still_image(fixture, map));
}

// Returns how many tiles the first view loads, and through `returning` how
// many it loads again after the view elsewhere. A null `tile_cache_size`
// leaves the option absent.
static int render_and_return(
  tile_server* server, const uint32_t* tile_cache_size, int* returning
) {
  server->bytes =
    mln_test_read_fixture("map/issue12432/0-0-0.mlt", &server->byte_count);
  TEST_ASSERT_NOT_NULL_MESSAGE(
    server->bytes, "MLT fixture missing; check tests/native/fixtures"
  );
  atomic_store(&server->tile_loads, 0);

  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_AWAIT_OK(mln_runtime_set_maximum_ambient_cache_size(
    runtime, 0, &completion.descriptor, NULL
  ));
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = serve_tile,
    .user_data = server,
  };
  MLN_TEST_AWAIT_OK(mln_runtime_set_resource_provider(
    runtime, &provider, &completion.descriptor, NULL
  ));

  mln_map_options options = mln_test_map_options();
  options.initial_extent =
    (mln_logical_extent){.width = 256, .height = 256, .scale_factor = 1.0};
  options.map_mode = MLN_MAP_MODE_STATIC;
  if (tile_cache_size != NULL) {
    options.fields |= MLN_MAP_OPTION_TILE_CACHE_SIZE;
    options.tile_cache_size = *tile_cache_size;
  }
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  mln_test_load_style_and_wait(map, MLN_BUFFER_LITERAL(tile_cache_style_json));
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  render_at(&fixture, map, 0.0, 0.0);
  const int first = atomic_load(&server->tile_loads);
  render_at(&fixture, map, 40.0, 100.0);
  const int before_return = atomic_load(&server->tile_loads);
  render_at(&fixture, map, 0.0, 0.0);
  *returning = atomic_load(&server->tile_loads) - before_return;

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  free(server->bytes);
  return first;
}

// The provider can run until the harness reclaims a runtime that a failing
// case leaves behind, so the server has static storage.
static tile_server server;

static void a_map_keeps_the_tiles_that_left_the_view_by_default(void) {
  int returning = -1;
  const int first = render_and_return(&server, NULL, &returning);
  TEST_ASSERT_GREATER_THAN_INT(0, first);
  TEST_ASSERT_EQUAL_INT(0, returning);
}

static void a_map_with_a_tile_cache_size_of_zero_loads_them_again(void) {
  const uint32_t size = 0;
  int returning = -1;
  const int first = render_and_return(&server, &size, &returning);
  TEST_ASSERT_GREATER_THAN_INT(0, first);
  TEST_ASSERT_EQUAL_INT(first, returning);
}

// The first view is centered where four tiles meet, so a cache of one keeps
// the last of them to leave the view.
static void a_map_with_a_tile_cache_size_of_one_keeps_one_tile(void) {
  const uint32_t size = 1;
  int returning = -1;
  const int first = render_and_return(&server, &size, &returning);
  TEST_ASSERT_GREATER_THAN_INT(1, first);
  TEST_ASSERT_EQUAL_INT(first - 1, returning);
}

MLN_TEST_GROUP {
  RUN_TEST(a_map_keeps_the_tiles_that_left_the_view_by_default);
  RUN_TEST(a_map_with_a_tile_cache_size_of_zero_loads_them_again);
  RUN_TEST(a_map_with_a_tile_cache_size_of_one_keeps_one_tile);
}
