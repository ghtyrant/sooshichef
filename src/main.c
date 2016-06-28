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

    GtkComboBox *trigger;
    GtkComboBox *sample_rate;
    GtkComboBox *sample_depth;

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

static void fill_combo_box_with_children(GtkComboBox *box, SooshiNode *node)
{
    GtkListStore *store = GTK_LIST_STORE(gtk_combo_box_get_model(box));

    GList *elem;
    SooshiNode *item;
    gint id = 0;
    for(elem = node->children; elem; elem = elem->next)
    {
        item = elem->data;

        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, item->name, 1, id, -1);

        id++;
    }

    gtk_combo_box_set_active_id(box, 0);
}

static void
sample_config_update(SooshiState *state, SooshiNode *node, gpointer user_data)
{
    gtk_combo_box_set_active(GTK_COMBO_BOX(user_data), g_variant_get_byte(node->value));
}

static void
sample_config_changed(GtkComboBox *box, gpointer user_data)
{
    AppState *s = (AppState*)user_data;

    gint chosen_value = gtk_combo_box_get_active(box);

    SooshiNode *base_node;
    if (box == s->trigger)
        base_node = sooshi_node_find(s->sooshi, "SAMPLING:TRIGGER", NULL);
    else if (box == s->sample_rate)
        base_node = sooshi_node_find(s->sooshi, "SAMPLING:RATE", NULL);
    else if (box == s->sample_depth)
        base_node = sooshi_node_find(s->sooshi, "SAMPLING:DEPTH", NULL);

    sooshi_node_choose_by_index(s->sooshi, base_node, chosen_value);
}

void mooshi_initialized(SooshiState *state, gpointer user_data)
{
    AppState *s = (AppState*)user_data;

    g_info("Mooshimeter initialized!");
    sooshi_node_subscribe(state, sooshi_node_find(state, "BAT_V", NULL), battery_update, user_data);
    sooshi_node_subscribe(state, sooshi_node_find(state, "CH1:VALUE", NULL), channel1_update, user_data);
    sooshi_node_subscribe(state, sooshi_node_find(state, "CH2:VALUE", NULL), channel2_update, user_data);

    SooshiNode *sampling_trigger    = sooshi_node_find(state, "SAMPLING:TRIGGER", NULL);
    SooshiNode *sampling_rate       = sooshi_node_find(state, "SAMPLING:RATE", NULL);
    SooshiNode *sampling_depth      = sooshi_node_find(state, "SAMPLING:DEPTH", NULL);

    fill_combo_box_with_children(s->trigger, sampling_trigger);
    fill_combo_box_with_children(s->sample_rate, sampling_rate);
    fill_combo_box_with_children(s->sample_depth, sampling_depth);

    sooshi_node_subscribe(state, sampling_trigger, sample_config_update, s->trigger);
    sooshi_node_subscribe(state, sampling_rate, sample_config_update, s->sample_rate);
    sooshi_node_subscribe(state, sampling_depth, sample_config_update, s->sample_depth);

    g_signal_connect(s->trigger, "changed", G_CALLBACK(sample_config_changed), s);
    g_signal_connect(s->sample_rate, "changed", G_CALLBACK(sample_config_changed), s);
    g_signal_connect(s->sample_depth, "changed", G_CALLBACK(sample_config_changed), s);

    sooshi_node_choose(state, sooshi_node_find(state, "SAMPLING:TRIGGER:CONTINUOUS", NULL));
    sooshi_node_choose(state, sooshi_node_find(state, "CH2:MAPPING:VOLTAGE", NULL));
    sooshi_node_choose(state, sooshi_node_find(state, "CH2:ANALYSIS:MEAN", NULL));
    sooshi_node_set_value(state, sooshi_node_find(state, "CH2:RANGE_I", NULL), g_variant_new_byte(0), TRUE);
}

void mooshi_scan_timed_out(SooshiState *state, gpointer user_data)
{

}

void app_shutdown(gpointer user_data)
{
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

    sooshi_error_t sooshi_error = 0;
    SooshiState *state = sooshi_state_new(&sooshi_error);

    if (sooshi_error)
        return -1;

    AppState *s = g_new0(AppState, 1);
    s->channel1_top    = gtk_builder_get_object(builder, "channel1_top");
    s->channel1        = gtk_builder_get_object(builder, "channel1");
    s->channel1_bottom = gtk_builder_get_object(builder, "channel1_bottom");

    s->channel2_top    = gtk_builder_get_object(builder, "channel2_top");
    s->channel2        = gtk_builder_get_object(builder, "channel2");
    s->channel2_bottom = gtk_builder_get_object(builder, "channel2_bottom");

    s->battery_level     = gtk_builder_get_object(builder, "battery_level");
    s->battery_level_str = gtk_builder_get_object(builder, "battery_level_str");

    s->trigger          = GTK_COMBO_BOX(gtk_builder_get_object(builder, "Trigger"));
    s->sample_rate      = GTK_COMBO_BOX(gtk_builder_get_object(builder, "Sample Rate"));
    s->sample_depth     = GTK_COMBO_BOX(gtk_builder_get_object(builder, "Sample Depth"));

    s->sooshi = state;

    GObject *window = gtk_builder_get_object(builder, "MainWindow");
    g_signal_connect(window, "destroy", G_CALLBACK(app_shutdown), s);

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

    sooshi_setup(state,
            mooshi_initialized, s,
            mooshi_scan_timed_out, s
    );

    gtk_widget_show_all(GTK_WIDGET(window));

    gtk_main();

    sooshi_state_delete(state);

    return 0;
}
