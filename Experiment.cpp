#include <iostream>

#include <uv.h>

using namespace std;
void wait_for_a_while(uv_idle_t* handle) {
    cout << "waiting ..." << endl;
}

int main() {
    uv_loop_t *loop = uv_default_loop();
    //uv_loop_init(loop);

    uv_idle_t idler;
    uv_idle_init(loop, &idler);
    uv_idle_start(&idler, wait_for_a_while);

    uv_run(loop, UV_RUN_DEFAULT);

    uv_loop_close(loop);
    return 0;
}
