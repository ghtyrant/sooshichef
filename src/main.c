#include <gtk/gtk.h>
#include <sooshi.h>

typedef struct
{
    GObject *channel1_top;
    GObject *channel1;
    GObject *channel1_bottom;

    GObject *channel2_top;
    GObject *channel2;
    GObject *channel2_bottom;

    GObject *battery_level;
    GObject *battery_level_str;

    SooshiState *sooshi;
} AppState;

void battery_update(SooshiState *state, SooshiNode *node, gpointer user_data)
{
    AppState *s = (AppState*)user_data;
    float voltage = g_variant_get_double(node->value);
    gtk_level_bar_set_value(GTK_LEVEL_BAR(s->battery_level), voltage - 2.0);

    gchar *str = g_strdup_printf("%.1f%%", (voltage - 2.0) * 100.0);
    gtk_label_set_text(GTK_LABEL(s->battery_level_str), str);
    g_free(str);
}

void channel1_update(SooshiState *state, SooshiNode *node, gpointer user_data)
{
    AppState *s = (AppState*)user_data;

    gchar *str = g_strdup_printf("%fA", g_variant_get_double(node->value));
    gtk_label_set_text(GTK_LABEL(s->channel1), str);
    g_free(str);
}

void channel2_update(SooshiState *state, SooshiNode *node, gpointer user_data)
{
    AppState *s = (AppState*)user_data;

    double value = g_variant_get_double(node->value);
    gchar *unit = "V";
    if (ABS(value) < 1.0)
    {
        value *= 1000.0;
        unit = "mV";
    }

    gchar *str = g_strdup_printf("%f %s", value, unit);
    gtk_label_set_text(GTK_LABEL(s->channel2), str);
    g_free(str);
}

void mooshi_initialized(SooshiState *state, gpointer user_data)
{
    g_info("Mooshimeter initialized!");
    sooshi_node_subscribe(state, sooshi_node_find(state, "BAT_V", NULL), battery_update, user_data);
    sooshi_node_subscribe(state, sooshi_node_find(state, "CH1:VALUE", NULL), channel1_update, user_data);
    sooshi_node_subscribe(state, sooshi_node_find(state, "CH2:VALUE", NULL), channel2_update, user_data);

    sooshi_node_choose(state, sooshi_node_find(state, "SAMPLING:TRIGGER:CONTINUOUS", NULL));
    sooshi_node_choose(state, sooshi_node_find(state, "CH2:MAPPING:VOLTAGE", NULL));
    sooshi_node_choose(state, sooshi_node_find(state, "CH2:ANALYSIS:MEAN", NULL));
    sooshi_node_set_value(state, sooshi_node_find(state, "CH2:RANGE_I", NULL), g_variant_new_byte(0), TRUE);
}

void app_shutdown(gpointer user_data)
{
    AppState *s = (AppState*)user_data;
    sooshi_state_delete(s->sooshi);
    gtk_main_quit();
}

int main(int argc, char *argv[])
{
    GtkBuilder *builder;

    gtk_init(&argc, &argv);

    builder = gtk_builder_new();
    GError *error = NULL;
    gtk_builder_add_from_file(builder, "src/ui/mainwindow.glade", &error);

    if (error != NULL)
    {
        printf("Error loading user interface: %s", error->message);
        g_error_free(error);
        return 0;
    }

    SooshiState *state = sooshi_state_new();
    AppState s;
    s.channel1_top    = gtk_builder_get_object(builder, "channel1_top");
    s.channel1        = gtk_builder_get_object(builder, "channel1");
    s.channel1_bottom = gtk_builder_get_object(builder, "channel1_bottom");

    s.channel2_top    = gtk_builder_get_object(builder, "channel2_top");
    s.channel2        = gtk_builder_get_object(builder, "channel2");
    s.channel2_bottom = gtk_builder_get_object(builder, "channel2_bottom");

    s.battery_level     = gtk_builder_get_object(builder, "battery_level");
    s.battery_level_str = gtk_builder_get_object(builder, "battery_level_str");

    s.sooshi = state;

    GObject *window = gtk_builder_get_object(builder, "MainWindow");
    g_signal_connect(window, "destroy", G_CALLBACK(app_shutdown), &s);

    /* Set up CSS style provider */
    GtkCssProvider *provider = gtk_css_provider_new();
    GdkDisplay *display = gdk_display_get_default();
    GdkScreen *screen = gdk_display_get_default_screen(display);

    gtk_style_context_add_provider_for_screen(screen,
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    gtk_css_provider_load_from_path(GTK_CSS_PROVIDER(provider),
        "src/ui/style.css", &error);

    if (error != NULL)
    {
        printf("Error loading user interface style: %s", error->message);
        g_error_free(error);
        return 0;
    }

    g_object_unref(provider);

    sooshi_setup(state, mooshi_initialized, &s);

    gtk_widget_show_all(GTK_WIDGET(window));

    gtk_main();

    return 0;
}
