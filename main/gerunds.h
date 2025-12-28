#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Array of gerund words for loading screen
static const char* gerunds[] = {
    "Accomplishing",
    "Actioning",
    "Actualizing",
    "Baking",
    "Booping",
    "Brewing",
    "Calculating",
    "Cerebrating",
    "Channelling",
    "Churning",
    "Clauding",
    "Coalescing",
    "Cogitating",
    "Combobulating",
    "Computing",
    "Concocting",
    "Conjuring",
    "Considering",
    "Contemplating",
    "Cooking",
    "Crafting",
    "Creating",
    "Crunching",
    "Deciphering",
    "Deliberating",
    "Determining",
    "Discombobulating",
    "Divining",
    "Doing",
    "Effecting",
    "Elucidating",
    "Enchanting",
    "Envisioning",
    "Finagling",
    "Flibbertigibbeting",
    "Forging",
    "Forming",
    "Frolicking",
    "Generating",
    "Germinating",
    "Hatching",
    "Herding",
    "Honking",
    "Hustling",
    "Ideating",
    "Imagining",
    "Incubating",
    "Inferring",
    "Jiving",
    "Manifesting",
    "Marinating",
    "Meandering",
    "Moseying",
    "Mulling",
    "Musing",
    "Mustering",
    "Noodling",
    "Percolating",
    "Perusing",
    "Philosophising",
    "Pondering",
    "Pontificating",
    "Processing",
    "Puttering",
    "Puzzling",
    "Reticulating",
    "Ruminating",
    "Scheming",
    "Schlepping",
    "Shimmying",
    "Shucking",
    "Simmering",
    "Smooshing",
    "Spelunking",
    "Spinning",
    "Stewing",
    "Sussing",
    "Synthesizing",
    "Thinking",
    "Tinkering",
    "Transmuting",
    "Unfurling",
    "Unravelling",
    "Vibing",
    "Wandering",
    "Whirring",
    "Wibbling",
    "Wizarding",
    "Working",
    "Wrangling"
};

#define GERUNDS_COUNT (sizeof(gerunds) / sizeof(gerunds[0]))

/**
 * Get a random gerund from the list
 * Uses esp_random() for randomization
 *
 * @return Pointer to a random gerund string
 */
const char* get_random_gerund(void);

#ifdef __cplusplus
}
#endif
